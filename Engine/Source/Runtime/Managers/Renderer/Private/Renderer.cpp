#include "../Public/Renderer.h"
#include <tbb/parallel_for_each.h>

#include "Source/Runtime/CommandPair/Public/CommandPair.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/DescriptorHeap/Public/Descriptors.h"
#include "Source/Runtime/Managers/CommonRenderer/Public/CommonRenderer.h"
#include "Source/Runtime/Managers/Debugger/Public/Debugger.hpp"
#include "Source/Runtime/Managers/ProjectionFrustum/Public/ProjectionFrustum.h"
#include "Source/Runtime/Managers/ReflectionEvaluator/Public/ReflectionEvaluator.h"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Managers/SceneManager/Public/SceneManager.hpp"
#include "Source/Runtime/Managers/Renderer/Public/ShadowManager.hpp"
#include "Source/Runtime/Resources/Material/Public/Material.h"

namespace Engine::Managers
{
	void Renderer::PreUpdate(const float& dt)
	{
		for (const auto& heap : m_tmp_descriptor_heaps_)
		{
			heap->Release();
		}

		m_tmp_descriptor_heaps_.clear();
		m_b_ready_ = false;
	}

	void Renderer::Update(const float& dt) {}

	void Renderer::FixedUpdate(const float& dt) {}

	void Renderer::PreRender(const float& dt)
	{
		// Pre-processing, Mapping the materials to the model renderers.
		const auto& scene = Managers::SceneManager::GetInstance().GetActiveScene().lock();
		BuildRenderMap(scene, m_render_candidates_, m_instance_count_);

		m_tmp_instance_buffers_.reset();
		m_tmp_instance_buffers_.resize(m_instance_count_);
		m_b_ready_ = true;
	}

	void Renderer::Render(const float& dt)
	{
		if (const auto scene = Managers::SceneManager::GetInstance().GetActiveScene().lock())
		{
			if (const auto cam = scene->GetMainCamera().lock())
			{
				const auto& cmd = Managers::D3Device::GetInstance().AcquireCommandPair(D3D12_COMMAND_LIST_TYPE_DIRECT, L"Main Rendering").lock();

				cmd->SoftReset();

				for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
				{
					// Check culling.
					RenderPass
							(
							 dt, static_cast<eShaderDomain>(i), false, cmd,
							 m_tmp_descriptor_heaps_,
							 m_tmp_instance_buffers_, [](const Strong<Abstracts::ObjectBase>& obj)
							 {
								 return Managers::ProjectionFrustum::GetInstance().CheckRender(obj);
							 }, [i](const Weak<CommandPair>& c, const DescriptorPtr& h)
							 {
								 Managers::D3Device::GetInstance().DefaultRenderTarget(c.lock()->GetList4());
								 Managers::RenderPipeline::GetInstance().DefaultViewport(c);
								 Managers::RenderPipeline::GetInstance().DefaultScissorRect(c);
								 Managers::ShadowManager::GetInstance().BindShadowMaps(c, h);
								 Managers::ShadowManager::GetInstance().BindShadowSampler(h);

								 if (i > SHADER_DOMAIN_OPAQUE)
								 {
									 Managers::ReflectionEvaluator::GetInstance().BindReflectionMap(c, h);
								 }
							 }, [](const Weak<CommandPair>& c, const DescriptorPtr& h)
							 {
								 Managers::ShadowManager::GetInstance().UnbindShadowMaps(c);
							 }, m_additional_structured_buffers_
							);

					if (i == SHADER_DOMAIN_OPAQUE)
					{
						// Notify reflection evaluator that rendering is finished so that it
						// can copy the rendered scene to the copy texture.
						Managers::ReflectionEvaluator::GetInstance().RenderFinished(cmd);
					}
				}

				Managers::ReflectionEvaluator::GetInstance().UnbindReflectionMap(cmd);

				cmd->FlagReady();
			}
		}
	}

	void Renderer::PostRender(const float& dt) {}

	void Renderer::PostUpdate(const float& dt) {}

	void Renderer::Initialize() {}

	void Renderer::AppendAdditionalStructuredBuffer(StructuredBufferBase* sb_ptr)
	{
		if (sb_ptr)
		{
			m_additional_structured_buffers_.push_back(sb_ptr);
		}
	}

	bool Renderer::Ready() const
	{
		return m_b_ready_;
	}

	/**
	 * \brief Render objects with the given domains.
	 * \param dt Delta time.
	 * \param domain Shader Domain.
	 * \param shader_bypass not use the material shader if flagged
	 * \param w_cmd Weak command pair pointer.
	 * \param descriptor_heap_container heap containers for the each instance, will be pushed back.
	 * \param instance_buffer_memory_pool instance container for the each instance, will be used index wise to optimize by not creating an instance buffer over and over again.
	 * \param predicate filter any object if it is not satisfied.
	 * \param initial_setup Initial setup for command list.
	 * \param post_setup Post setup for command list.
	 * \param additional_structured_buffers structured buffer to bind.
	 */
	void Renderer::RenderPass
	(
		const float                                                      dt,
		const eShaderDomain                                              domain,
		bool                                                             shader_bypass,
		const Weak<CommandPair>&                                         w_cmd,
		DescriptorContainer&                                             descriptor_heap_container,
		StructuredBufferMemoryPool<SBs::InstanceSB>&                     instance_buffer_memory_pool,
		const std::function<bool(const Strong<Abstracts::ObjectBase>&)>& predicate,
		const CommandPairExtension::CommandDescriptorLambda&             initial_setup,
		const CommandPairExtension::CommandDescriptorLambda&             post_setup,
		const std::vector<StructuredBufferBase*>&                        additional_structured_buffers = {}
	)
	{
		if (!Ready())
		{
			Managers::Debugger::GetInstance().Log("Renderer is not ready for rendering!");
			return;
		}

		if (m_render_candidates_[domain].empty())
		{
			return;
		}

		const auto&                                                     target_set = m_render_candidates_[domain];
		tbb::concurrent_hash_map<Weak<Resources::Material>, aligned_vector<const SBs::InstanceSB*>> final_mapping;

		for (const auto& mtr_m : target_set | std::views::values)
		{
			tbb::parallel_for_each
					(
					 mtr_m.begin(), mtr_m.end(), [&](const CandidateTuple& tuple)
					 {
						 if (!predicate || predicate(std::get<0>(tuple).lock()))
						 {
							 decltype(final_mapping)::accessor acc;

							 if (!final_mapping.find(acc, std::get<1>(tuple)))
							 {
								 final_mapping.insert(acc, std::get<1>(tuple));
							 }

							 for (const SBs::InstanceSB& instance : std::get<2>(tuple))
							 {
								 acc->second.push_back(&instance);
							 }
						 }
					 }
					);
		}

		const auto& cmd = w_cmd.lock();

		for (const auto& sb_ptr : additional_structured_buffers)
		{
			if (sb_ptr)
			{
				sb_ptr->TransitionToSRV(cmd->GetList());
			}
		}

		for (const auto& [mtr, sbs] : final_mapping)
		{
			descriptor_heap_container.emplace_back(Managers::RenderPipeline::GetInstance().AcquireHeapSlot().lock());

			const auto& heap = descriptor_heap_container.back();

			initial_setup(cmd, heap);

			for (const auto& sb_ptr : additional_structured_buffers)
			{
				if (sb_ptr)
				{
					sb_ptr->CopySRVHeap(heap);
				}
			}

			heap->BindGraphic(cmd->GetList4());

			auto& instance = instance_buffer_memory_pool.get();

			renderPassImpl(dt, domain, shader_bypass, instance, mtr.lock(), cmd, heap, sbs);

			post_setup(cmd, heap);

			instance_buffer_memory_pool.advance();
		}

		for (const auto& sb_ptr : additional_structured_buffers)
		{
			if (sb_ptr)
			{
				sb_ptr->TransitionCommon(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			}
		}
	}

	UINT64 Renderer::GetInstanceCount() const
	{
		if (!m_b_ready_)
		{
			throw std::runtime_error("Renderer is not ready for rendering!");
		}

		return m_instance_count_;
	}

	void Renderer::renderPassImpl(
		const float                         dt,
		eShaderDomain                       domain,
		bool                                shader_bypass,
		StructuredBuffer<SBs::InstanceSB>&  instance_buffer,
		const Strong<Resources::Material>&               material,
		const Weak<CommandPair>&            w_cmd,
		const DescriptorPtr&                heap,
		const aligned_vector<const SBs::InstanceSB*>& structured_buffers
	)
	{
		const auto& cmd = w_cmd.lock();

		CheckSize<UINT>(structured_buffers.size(), L"Warning: Renderer will take a lot of amount of instance buffers!");
		instance_buffer.SetDataContainer(cmd->GetList(), static_cast<UINT>(structured_buffers.size()), structured_buffers.data());
		instance_buffer.TransitionToSRV(cmd->GetList());
		instance_buffer.CopySRVHeap(heap);

		material->SetTempParam
				(
				 {
					 .instanceCount = static_cast<UINT>(structured_buffers.size()),
					 .bypassShader = shader_bypass,
					 .domain = domain,
				 }
				);

		material->Draw(dt, cmd, heap);

		instance_buffer.TransitionCommon(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}
}

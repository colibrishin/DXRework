#include "RenderPassTaskDX12.h"

#include <ranges>
#include <tbb/parallel_for_each.h>

#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"
#include "Source/Runtime/Managers/RenderPipeline//Public/RenderPipeline.h"
#include "Source/Runtime/Resources/Shape/Public/Shape.h"
#include "Source/Runtime/Resources/Texture/Public/Texture.h"
#include "Source/Runtime/PrimitivePipelineDX12/Public/PrimitivePipelineDX12.h"

namespace Engine 
{
	void Engine::DX12RenderPassTask::Run(
		const float dt,
		const bool shader_bypass,
		const eShaderDomain domain,
		RenderMap const* domain_map,
		const Graphics::SBs::LocalParamSB& local_param,
		const std::atomic<uint64_t>& instance_count,
		RenderPassPrerequisiteTask* const* prerequisite,
		const size_t prerequisite_count,
		const ObjectPredication& predicate)
	{
		if (domain_map->empty())
		{
			return;
		}

		tbb::concurrent_hash_map<Weak<Resources::Material>, aligned_vector<const Graphics::SBs::InstanceSB*>> final_mapping;

		for (const auto& mtr_m : *domain_map | std::views::values)
		{
			tbb::parallel_for_each
			(
				mtr_m.begin(), mtr_m.end(), [&](const CandidateTuple& tuple)
				{
					if (!predicate || (predicate && predicate(std::get<0>(tuple).lock())))
					{
						decltype(final_mapping)::accessor acc;

						if (!final_mapping.find(acc, std::get<1>(tuple)))
						{
							final_mapping.insert(acc, std::get<1>(tuple));
						}

						for (const Graphics::SBs::InstanceSB& instance : std::get<2>(tuple))
						{
							acc->second.push_back(&instance);
						}
					}
				}
			);
		}

		GraphicInterface& gi = Managers::RenderPipeline::GetInstance().GetInterface();
		const GraphicInterfaceContextReturnType& context = gi.GetNewContext(0, false, L"Render Pass");
		const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();
		
		primitive.commandList->SoftReset();

		StructuredBufferTypeBase<Graphics::SBs::LocalParamSB>& sb = m_local_param_pool_->get();
		sb.SetData(&primitive, 1, &local_param);
		sb.TransitionToSRV(&primitive);

		for (const auto& [mtr, sbs] : final_mapping)
		{
			m_heaps_.emplace_back(gi.GetHeap());
			m_current_heap_ = m_heaps_.back().get();

			GraphicInterfaceContextPrimitive temp_context
			{
				.commandList = primitive.commandList,
				.heap = m_current_heap_
			};

			for (size_t i = 0; i < prerequisite_count; ++i)
			{
				prerequisite[i]->Run(this);
			}

			sb.CopySRVHeap(&temp_context);
			m_current_heap_->BindGraphic(primitive.commandList);

			StructuredBufferTypeBase<Graphics::SBs::InstanceSB>& instance = m_instance_pool_->get();
			RunImpl(dt, shader_bypass, domain, instance, mtr.lock(), cmd, heap, sbs);

			for (size_t i = 0; i < prerequisite_count; ++i)
			{
				prerequisite[i]->Cleanup(this);
			}

			m_instance_pool_->advance();

		}

		sb.TransitionCommon(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		cmd->FlagReady();

		m_local_param_pool_.advance();
	}

	void Engine::DX12RenderPassTask::Cleanup()
	{
		m_updated_material_in_current_pass_.clear();
		m_local_param_pool_.reset();
		m_instance_pool_.reset();
	}

	void Engine::DX12RenderPassTask::RunImpl(const float dt, const bool shader_bypass, const eShaderDomain shader_domain, Graphics::StructuredBuffer<Graphics::SBs::InstanceSB>& instance_buffer, const Weak<Resources::Material>& material, const Weak<CommandPair>& w_cmd, const DescriptorPtr& w_heap, const aligned_vector<const Graphics::SBs::InstanceSB*>& structuredbuffers)
	{
		const Strong<CommandPair>& cmd = w_cmd.lock();
		const StrongDescriptorPtr heap = w_heap.lock();

		CheckSize<UINT>(structuredbuffers.size(), L"Warning: Renderer will take a lot of amount of instance buffers!");
		instance_buffer.SetDataContainer(cmd->GetList(), static_cast<UINT>(structuredbuffers.size()), structuredbuffers.data());
		instance_buffer.TransitionToSRV(cmd->GetList());
		instance_buffer.CopySRVHeap(heap.get());

		DrawPhase(shader_bypass, structuredbuffers.size(), material, w_cmd, w_heap);

		instance_buffer.TransitionCommon(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}

	void Engine::DX12RenderPassTask::DrawPhase(
		const bool shader_bypass,
		const uint64_t instance_count,
		const Weak<Resources::Material>& material,
		const Weak<CommandPair>& w_cmd,
		const DescriptorPtr& w_heap)
	{
		if (const Strong<Resources::Material>& mat = material.lock())
		{
			if (mat->GetResource<Resources::Shape>(0).expired())
			{
				return;
			}

			const Graphics::SBs::MaterialSB& mat_sb = mat->GetMaterialSB();
			const auto& material_resources = mat->GetResources();

			const auto& cmd = w_cmd.lock();
			const auto& heap = w_heap.lock();

			heap->BindGraphic(cmd->GetList4());

			if (!shader_bypass)
			{
				const Strong<Resources::Shader>& shader = mat->GetResource<Resources::Shader>(0).lock();
				GraphicPrimitiveShader* primitive = shader->GetPrimitiveShader();
				ShaderRenderPrerequisiteTask& task = primitive->GetShaderPrerequisiteTask();
				task.Run(this);
			}

			for (const auto& [type, resources] : material_resources)
			{
				if (type == RES_T_ATLAS_TEX)
				{
					const auto& anim = resources.front()->GetSharedPtr<Resources::Texture>();
					TextureBindingTask& task = anim->GetPrimitiveTexture()->GetBindingTask();
					task.Bind(this, anim->GetPrimitiveTexture(), BIND_TYPE_SRV, RESERVED_TEX_ATLAS, 0);
					continue;
				}

				if (type == RES_T_TEX) 
				{
					for (auto it = resources.begin(); it != resources.end(); ++it)
					{
						const auto& res = *it;

						// todo: distinguish tex type
						const UINT  idx = static_cast<UINT>(std::distance(resources.begin(), it));
						const auto& tex = res->GetSharedPtr<Resources::Texture>();

						TextureBindingTask& task = tex->GetPrimitiveTexture()->GetBindingTask();
						task.Bind(this, tex->GetPrimitiveTexture(), BIND_TYPE_SRV, BIND_SLOT_TEX, idx);
					}
				}
			}

			if (!m_updated_material_in_current_pass_.contains(reinterpret_cast<uint64_t>(mat.get()))) 
			{
				m_material_sbs_[reinterpret_cast<uint64_t>(mat.get())].SetData(cmd->GetList(), 1, &mat_sb);
				m_updated_material_in_current_pass_.insert(reinterpret_cast<uint64_t>(mat.get()));
			}

			m_material_sbs_[reinterpret_cast<uint64_t>(mat.get())].TransitionToSRV(cmd->GetList());
			m_material_sbs_[reinterpret_cast<uint64_t>(mat.get())].CopySRVHeap(heap.get());

			if (const Strong<Resources::Shape>& shape = mat->GetResource<Resources::Shape>(0).lock())
			{
				if (const auto& anim = shape->GetAnimations().lock())
				{
					TextureBindingTask& task = anim->GetPrimitiveTexture()->GetBindingTask();
					task.Bind(this, anim->GetPrimitiveTexture(), BIND_TYPE_SRV, RESERVED_TEX_BONES, 0);
				}

				for (const auto& mesh : shape->GetMeshes())
				{
					const auto& idx_count = mesh->GetIndexCount();
					const auto& vtx_view = mesh->GetVertexView();
					const auto& idx_view = mesh->GetIndexView();

					cmd->GetList()->IASetVertexBuffers(0, 1, &vtx_view);
					cmd->GetList()->IASetIndexBuffer(&idx_view);
					CheckSize<UINT>(idx_count, L"Warning: DrawIndexedInstance will takes large size of indices!");
					cmd->GetList()->DrawIndexedInstanced(static_cast<UINT>(idx_count), instance_count, 0, 0, 0);
				}

				if (const auto& anim = shape->GetAnimations().lock())
				{
					PrimitiveTexture* primitive = anim->GetPrimitiveTexture();
					primitive->GetBindingTask().Unbind(this, primitive, BIND_TYPE_SRV);
				}
			}

			if (material_resources.contains(RES_T_TEX))
			{
				for (const auto& tex : material_resources.at(RES_T_TEX))
				{
					PrimitiveTexture* primitive = tex->GetSharedPtr<Resources::Texture>()->GetPrimitiveTexture();
					primitive->GetBindingTask().Unbind(this, primitive, BIND_TYPE_SRV);
				}
			}

			if (material_resources.contains(RES_T_ATLAS_TEX))
			{
				PrimitiveTexture* primitive = material_resources.at(RES_T_ATLAS_TEX).front()->GetSharedPtr<Resources::Texture>()->GetPrimitiveTexture();
				primitive->GetBindingTask().Unbind(this, primitive, BIND_TYPE_SRV);
			}

			m_material_sbs_[reinterpret_cast<uint64_t>(mat.get())].TransitionCommon(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		}
	}
}
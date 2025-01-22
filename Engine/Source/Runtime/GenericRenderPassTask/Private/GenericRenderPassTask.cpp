#include "../Public/GenericRenderPassTask.h"

#include <ranges>
#include <tbb/parallel_for_each.h>

#include "RenderPipeline.h"
#include "Renderer.h"

#include "Source/Runtime/Resources/AtlasAnimationTexture/Public/AtlasAnimationTexture.h"
#include "Source/Runtime/Resources/Material/Public/Material.h"
#include "Source/Runtime/Resources/Shape/Public/Shape.h"

namespace Engine
{
	GenericRenderPassTask::GenericRenderPassTask()
		: m_gi_ticket_(SingletonSpinLock::GetInstance().Register()),
		  m_local_param_pool_ticket(SingletonSpinLock::GetInstance().Register()),
		  m_instance_pool_ticket(SingletonSpinLock::GetInstance().Register()) {}

	void GenericRenderPassTask::Run(
		const float                        dt,
		const bool                         shader_bypass,
		RenderMap const*                   domain_map,
		const Graphics::SBs::LocalParamSB& local_param,
		const ObjectPredication&           predicate,
		const ContextSetupFunction&        prerender_predicate,
		const ContextSetupFunction&        postrender_predicate
	)
	{
		if (domain_map->empty())
		{
			return;
		}

		// Filter the instances by the predicate
		IntermediateMeshMap intermediate_mesh_map = PredicateObject(predicate, domain_map);

		for (const auto& renderer : *domain_map | std::views::values)
		{
			for (const auto& [mesh, shader_map] : renderer)
			{
				tbb::parallel_for_each(shader_map.begin(), shader_map.end(),
					[weak_mesh = Weak(mesh), this, &local_param, &prerender_predicate, &postrender_predicate, &shader_bypass, &dt, &intermediate_mesh_map](const std::pair<Strong<Resources::Shader>, aligned_vector<InstancePair>>& pair)
				{
					// todo: possible bottleneck?
					if (decltype(intermediate_mesh_map)::const_accessor acc; intermediate_mesh_map.find(acc, weak_mesh))
					{
						StartPhase_MultiThread(dt, shader_bypass, weak_mesh, pair.first, local_param, prerender_predicate, postrender_predicate, acc->second);
					}
				});
			}
		}
	}

	void GenericRenderPassTask::Cleanup()
	{
		m_local_param_pool_.reset();
		m_instance_pool_.reset();
	}

	GenericRenderPassTask::IntermediateMeshMap GenericRenderPassTask::PredicateObject(const ObjectPredication& predicate, RenderMap const* domain_map)
	{
		IntermediateMeshMap final_mapping;
		
		for (const auto& meshes : *domain_map | std::views::values)
		{
			tbb::parallel_for_each
			(
				meshes.begin(), meshes.end(), [&](const std::pair<Strong<Resources::Mesh>, ShaderMap>& mesh_pair)
				{
					for (const aligned_vector<InstancePair>& instances : mesh_pair.second | std::views::values)
					{
						for (const InstancePair& instance_pair : instances)
						{
							if (predicate && !predicate(instance_pair.object))
							{
								continue;
							}

							decltype(final_mapping)::accessor acc;
							if (!final_mapping.find(acc, mesh_pair.first))
							{
								final_mapping.insert(acc, mesh_pair.first);
							}

							acc->second.push_back(&instance_pair);
						}
					}
				}
			);
		}

		return final_mapping;
	}

	void GenericRenderPassTask::StartPhase_MultiThread(
		const float dt,
		const bool shader_bypass,
		const Weak<Resources::Mesh>& mesh,
		const Weak<Resources::Shader>& shader,
		const Graphics::SBs::LocalParamSB& local_param,
		const ContextSetupFunction& prerender_predicate,
		const ContextSetupFunction& postrender_predicate,
		const aligned_vector<const InstancePair*>& instance_pairs)
	{
		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();

		// Manual release
		SpinLockToken gi_token = SingletonSpinLock::GetInstance().Lock(m_gi_ticket_);
		const GraphicInterfaceContextReturnType& context = std::move(gi.GetNewContext(0, false, L"Render Pass"));
		gi_token.Release();
		
		const GraphicInterfaceContextPrimitive&  primitive = context.GetPointers();

		primitive.commandList->SoftReset();

		// Manual release
		SpinLockToken local_param_token = SingletonSpinLock::GetInstance().Lock(m_local_param_pool_ticket);
		StructuredBufferTypeProxy<Graphics::SBs::LocalParamSB>& sb          = m_local_param_pool_.get();
		local_param_token.Release();
		
		StructuredBufferTypelessBase&                           sb_typeless = sb.GetTypeless();
		sb.SetData(&primitive, 1, &local_param);
		sb_typeless.TransitionToSRV(&primitive);
		Managers::RenderPipeline::GetInstance().BindConstantBuffers(&primitive);
		gi.SetDefaultRenderTarget(&primitive);

		GraphicHeapBase*                 current_heap = m_heaps_.emplace_back(gi.GetHeap())->get();
		GraphicInterfaceContextPrimitive temp_context
		{
			.commandList = primitive.commandList,
			.heap = current_heap
		};

		if (prerender_predicate) { prerender_predicate(&temp_context); }

		sb.CopySRVHeap(&temp_context);
		current_heap->BindGraphic(&primitive);

		StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>& instance = m_instance_pool_.get();
		
		static aligned_vector<Graphics::SBs::InstanceSB*> instances;
		static aligned_vector<TexturePair> texture_pairs;
		
		if (instance_pairs.size() > instances.capacity())
		{
			instances.resize(instance_pairs.size());
			texture_pairs.resize(instance_pairs.size());
		}

		size_t idx = 0;
		for (const InstancePair* instance_pair : instance_pairs)
		{
			instances[idx] = instance_pair->instance;
			texture_pairs[idx] = TexturePair(&instance_pair->textures, &instance_pair->reservedTextures);
			++idx;
		}
		
		DrawPhase_MultiThread(dt, shader_bypass, idx, instance, shader, mesh, &temp_context, instances, texture_pairs);

		if (postrender_predicate) { postrender_predicate(&temp_context); }
		
		{
			auto token = SingletonSpinLock::GetInstance().Lock(m_instance_pool_ticket);
			m_instance_pool_.advance();
		}

		sb_typeless.TransitionCommon(&primitive);
		primitive.commandList->FlagReady();

		{
			auto token = SingletonSpinLock::GetInstance().Lock(m_local_param_pool_ticket);
			m_local_param_pool_.advance();
		}
	}

	void GenericRenderPassTask::DrawPhase_MultiThread(
		float dt,
		bool shader_bypass,
		const size_t instance_count,
		StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>& instance_buffer,
		const Weak<Resources::Shader>& shader,
		const Weak<Resources::Mesh>& mesh,
		const GraphicInterfaceContextPrimitive* context,
		const aligned_vector<Graphics::SBs::InstanceSB*>& instances,
		const aligned_vector<TexturePair>& texture_pairs
	)
	{
		CheckSize<UINT>(instance_count, L"Warning: Renderer will take a lot of amount of instance buffers!");
		context->heap->BindGraphic(context);

		// Manual release
		auto token = SingletonSpinLock::GetInstance().Lock(m_gi_ticket_);
		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		token.Release();

		const Strong<Resources::Shader>& locked_shader = shader.lock();
		if (!shader_bypass)
		{
			gi.BindGraphic(context, locked_shader.get());
		}

		size_t instance_resolved = 0;
		while (instance_resolved != instance_count)
		{
			assert(instance_resolved > instance_count);
			
			size_t instance_to_resolve = 0;

			constexpr size_t max_tex_binds = BIND_SLOT_TEXARR - BIND_SLOT_TEX;
			uint16_t tex_bind_mask = 0; // See max tex binds

			while (const uint16_t count = _tzcnt_u16(tex_bind_mask))
			{
				const TexturePair& pair = texture_pairs[instance_resolved];
				if (pair.reservedTextures->size())
				{
					// independent draw call, no redundant specific textures slots.
					constexpr size_t offset = 0;
					for (size_t i = 0; i < pair.textures->size(); ++i)
					{
						tex_bind_mask |= (1 << (offset + i));
					}
					instances[offset]->SetTextureOffset(offset);
					instance_to_resolve++;
					break;
				}
				
				if (!pair.reservedTextures->size() && count > pair.textures->size())
				{
					const size_t offset = max_tex_binds - count;
					for (size_t i = 0; i < pair.textures->size(); ++i)
					{
						tex_bind_mask |= (1 << (offset + i));
					}
					instances[offset]->SetTextureOffset(offset);
					instance_to_resolve++;
				}
			}

			instance_buffer.SetDataContainer(context, static_cast<UINT>(instance_to_resolve), instances.data() + instance_resolved);
			StructuredBufferTypelessBase& typeless = instance_buffer.GetTypeless();
			typeless.TransitionToSRV(context);
			instance_buffer.CopySRVHeap(context);

			const Strong<Resources::Mesh>& locked_mesh = mesh.lock();

			UINT total_offset = 0;
			for (size_t i = 0; i < instance_to_resolve; ++i)
			{
				for (size_t j = 0; j < texture_pairs[instance_resolved + i].textures->size(); ++j)
				{
					const auto& tex = texture_pairs[instance_resolved + i].textures->at(j);
					gi.Bind(context, tex.get(), BIND_TYPE_SRV, BIND_SLOT_TEX, total_offset);
					++total_offset;
				}
				
				if (texture_pairs[instance_resolved + i].reservedTextures->size())
				{
					for (size_t j = 0; j < texture_pairs[instance_resolved + i].reservedTextures->size(); ++j)
					{
						const auto& tex = texture_pairs[instance_resolved + i].reservedTextures->at(j);
						if (tex->GetTypeHash() == Resources::AtlasAnimationTexture::StaticTypeHash())
						{
							gi.Bind(context, tex.get(), BIND_TYPE_SRV, RESERVED_USER_TEX_ATLAS, 0);
						}
						else if (tex->GetTypeHash() == Resources::AnimationTexture::StaticTypeHash())
						{
							gi.Bind(context, tex.get(), BIND_TYPE_SRV, RESERVED_USER_TEX_BONES, 0);
						}
					}
				}
			}

			gi.Draw(context, locked_mesh.get(), instance_to_resolve, instance_resolved);

			for (size_t i = 0; i < instance_to_resolve; ++i)
			{
				for (size_t j = 0; j < texture_pairs[instance_resolved + i].textures->size(); ++j)
				{
					const auto& tex = texture_pairs[instance_resolved + i].textures->at(j);
					gi.Unbind(context, tex.get(), BIND_TYPE_SRV);
				}

				if (texture_pairs[instance_resolved + i].reservedTextures->size())
				{
					for (size_t j = 0; j < texture_pairs[instance_resolved + i].reservedTextures->size(); ++j)
					{
						const auto& tex = texture_pairs[instance_resolved + i].reservedTextures->at(j);
						if (tex->GetTypeHash() == Resources::AtlasAnimationTexture::StaticTypeHash())
						{
							gi.Unbind(context, tex.get(), BIND_TYPE_SRV);
						}
						else if (tex->GetTypeHash() == Resources::AnimationTexture::StaticTypeHash())
						{
							gi.Unbind(context, tex.get(), BIND_TYPE_SRV);
						}
					}
				}
			}

			typeless.TransitionCommon(context);
			instance_resolved += instance_to_resolve;
		}
	}
}

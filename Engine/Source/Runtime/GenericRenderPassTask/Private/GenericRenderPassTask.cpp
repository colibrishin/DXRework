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
		  m_instance_pool_ticket(SingletonSpinLock::GetInstance().Register()),
		  m_texture_record_ticket_(SingletonSpinLock::GetInstance().Register()) {}

	void GenericRenderPassTask::Run(
		float                                                             dt,
		bool                                                              shader_bypass,
		RenderMap const*                                                  domain_map,
		const aligned_vector<const StructuredBufferDecorator*>&           additional_sbs,
		const Graphics::SBs::LocalParamSB&                                local_param,
		const ObjectPredication&                                          predicate,
		const ContextSetupFunction&                                       prerender_predicate,
		const ContextSetupFunction&                                       postrender_predicate,
		const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_predicates,
		const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_predicates
	)
	{
		for (auto& tex : m_used_shader_textures_)
		{
			tex = nullptr;
		}
		
		if (domain_map->empty())
		{
			return;
		}
		
		// Filter the instances by the predicate
		uint64_t instance_count = 0;
		IntermediateMeshMap intermediate_mesh_map;
		PredicateObject(predicate, domain_map, instance_count, intermediate_mesh_map);

		m_local_param_pool_.Update(nullptr, instance_count);
		m_instance_pool_.Update(nullptr, instance_count);

		for (const auto& renderer : *domain_map | std::views::values)
		{
			for (const auto& [mesh, shader_map] : renderer)
			{
				tbb::parallel_for_each(shader_map.begin(), shader_map.end(),
					[weak_mesh = Weak(mesh), this, &local_param, &postrender_predicates, &prerender_predicates, &prerender_predicate, &postrender_predicate, &additional_sbs, &shader_bypass, &dt, &intermediate_mesh_map](const std::pair<Strong<Resources::Shader>, aligned_vector<InstancePair>>& pair)
				{
					// todo: possible bottleneck?
					if (decltype(intermediate_mesh_map)::const_accessor acc; intermediate_mesh_map.find(acc, weak_mesh))
					{
						StartPhase_MultiThread(dt, shader_bypass, weak_mesh, pair.first, additional_sbs, local_param, prerender_predicate, postrender_predicate, prerender_predicates, postrender_predicates, acc->second);
					}
				});
			}
		}

		auto& gi = GraphicInterfaceAccessor::GetInterface();
		auto context = gi.GetNewContext(0, false, L"Lazy Shader Resource Texture Transition Back");
		auto primitive = context.GetPointers();
		
		primitive.commandList->SoftReset();
		const auto& range = std::ranges::unique(m_used_shader_textures_);
		const size_t indeterminate = std::distance(range.begin(), range.begin());
		const size_t unique_idx = m_used_shader_textures_.size() - indeterminate;
		if (m_used_shader_textures_.size() > 0 && m_used_shader_textures_[0] != nullptr)
		{
			gi.TransitBackMultiple(&primitive, m_used_shader_textures_.data(), unique_idx, BIND_TYPE_SRV);
		}
		primitive.commandList->FlagReady();
	}

	void GenericRenderPassTask::Cleanup()
	{
		m_local_param_pool_.reset();
		m_instance_pool_.reset();
		m_heaps_.clear(); // todo: reuse
		std::ranges::fill(m_used_shader_textures_, nullptr);
	}

	void GenericRenderPassTask::PredicateObject(const ObjectPredication& predicate, RenderMap const* domain_map, uint64_t& instance_count, IntermediateMeshMap& out_map) const
	{
		out_map.clear();
		
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

							std::remove_reference_t<decltype(out_map)>::accessor acc;
							if (!out_map.find(acc, mesh_pair.first))
							{
								out_map.insert(acc, mesh_pair.first);
							}

							acc->second.push_back(instance_pair);
							++instance_count;
						}
					}
				}
			);
		}
	}

	void GenericRenderPassTask::StartPhase_MultiThread(
		const float                                                       dt,
		const bool                                                        shader_bypass,
		const Weak<Resources::Mesh>&                                      mesh,
		const Weak<Resources::Shader>&                                    shader,
		const aligned_vector<const StructuredBufferDecorator*>&           additional_sbs,
		const Graphics::SBs::LocalParamSB&                                local_param,
		const ContextSetupFunction&                                       prerender_predicate,
		const ContextSetupFunction&                                       postrender_predicate,
		const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_predicates,
		const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_predicates,
		const aligned_vector<InstancePair>&                               instance_pairs
	)
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
		m_local_param_pool_.advance();
		StructuredBufferTypeProxy<Graphics::SBs::LocalParamSB>& sb = m_local_param_pool_.get();
		local_param_token.Release();

		GraphicHeapBase*                       current_heap = m_heaps_.emplace_back(gi.GetHeap())->get();
		const GraphicInterfaceContextPrimitive temp_context
		{
			.commandList = primitive.commandList,
			.heap = current_heap
		};

		current_heap->BindGraphic(&temp_context);
		sb.SetData(&temp_context, 1, &local_param);
		sb.TransitionToSRV(&temp_context);
		gi.SetDefaultRenderTarget(&temp_context);
		sb.CopySRVHeap(&temp_context);
		Managers::RenderPipeline::GetInstance().BindConstantBuffers(&temp_context);
		gi.SetViewport(&temp_context, Managers::RenderPipeline::GetInstance().GetViewport());

		if (prerender_predicate) { prerender_predicate(&temp_context); }

		for (const auto& func : prerender_predicates | std::views::values)
		{
			func(&temp_context);
		}
		
		// Manual release
		auto instance_token = SingletonSpinLock::GetInstance().Lock(m_instance_pool_ticket);
		m_instance_pool_.advance();
		StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>& instance = m_instance_pool_.get();
		instance_token.Release();
		
		static aligned_vector<Graphics::SBs::InstanceSB*> instances;
		static aligned_vector<TexturePair> texture_pairs;
		
		if (instance_pairs.size() > instances.capacity())
		{
			instances.resize(instance_pairs.size());
			texture_pairs.resize(instance_pairs.size());
		}

		size_t idx = 0;
		for (const InstancePair& instance_pair : instance_pairs)
		{
			instances[idx] = instance_pair.instance;
			texture_pairs[idx] = TexturePair(&instance_pair.textures, &instance_pair.reservedTextures);
			++idx;
		}

		for (const StructuredBufferDecorator* additional_sb : additional_sbs)
		{
			additional_sb->TransitionToSRV(&temp_context);
			additional_sb->CopySRVHeap(&temp_context);
		}
		
		DrawPhase_MultiThread(dt, shader_bypass, idx, instance, shader, mesh, &temp_context, instances, texture_pairs);

		if (postrender_predicate) { postrender_predicate(&temp_context); }

		for (const auto& func : postrender_predicates | std::views::values)
		{
			func(&temp_context);
		}
		
		for (const StructuredBufferDecorator* additional_sb : additional_sbs)
		{
			additional_sb->TransitionCommon(&temp_context);
		}

		sb.TransitionCommon(&temp_context);
		temp_context.commandList->FlagReady();
	}

	void GenericRenderPassTask::RecordUsedTexture(
		const GraphicInterfaceContextPrimitive* context, GraphicInterface& gi, const Strong<Resources::Texture>& tex
	)
	{
		auto tt = SingletonSpinLock::GetInstance().Lock(m_texture_record_ticket_);
		if (std::ranges::find(m_used_shader_textures_, tex.get()) == m_used_shader_textures_.end())
		{
			gi.TransitTo(context, tex.get(), BIND_TYPE_SRV);
			if (const auto& empty_slot = std::ranges::find(m_used_shader_textures_, nullptr);
				empty_slot == m_used_shader_textures_.end())
			{
				m_used_shader_textures_.push_back(tex.get());	
			}
			else
			{
				*empty_slot = tex.get();
			}
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
			size_t instance_to_resolve = 0;

			constexpr size_t max_tex_binds = BIND_SLOT_TEXARR - BIND_SLOT_TEX;
			uint16_t tex_bind_mask = 0; // See max tex binds
			std::array<Strong<Resources::Texture>, std::numeric_limits<uint16_t>::digits> assigned_texture;
			std::array<Strong<Resources::Texture>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN> reserved_textures;
			
			while (const uint16_t count = _tzcnt_u16(tex_bind_mask))
			{
				if (instance_resolved + instance_to_resolve == instance_count)
				{
					break;
				}
				
				const TexturePair& pair = texture_pairs[instance_resolved];
				bool reserved_texture_tolerant = false;

				for (size_t i = 0; i < pair.reservedTextures->size(); ++i)
				{
					if (!pair.reservedTextures->at(i))
					{
						continue;
					}
					if (pair.reservedTextures->at(i))
					{
						if (reserved_textures[i] == nullptr)
						// allow to instance with the first reserved texture encountered.
						reserved_texture_tolerant = true;
						reserved_textures[i] = pair.reservedTextures->at(i);
					}
					else if (pair.reservedTextures->at(i) == reserved_textures[i] && reserved_textures[i] != nullptr)
					{
						// tolerance the same reserved texture.
						reserved_texture_tolerant = true;
					}
					else
					{
						// another reserved texture for the same slot.
						// unable to handle, split the instancing
						break;
					}
				}
				
				if (count > pair.textures->size())
				{
					size_t msb = count - 1;
					const size_t lsb = max_tex_binds - count;
					for (size_t i = 0; i < pair.textures->size(); ++i)
					{
						if (pair.textures->at(i))
						{
							if (const auto& it = std::ranges::find(assigned_texture, pair.textures->at(i));
								it != assigned_texture.end())
							{
								const size_t bind_slot = std::distance(assigned_texture.begin(), it);
								instances[instance_resolved + instance_to_resolve]->SetTextureSlot(i, bind_slot);
							}
							else
							{
								tex_bind_mask |= 1 << msb;
								assigned_texture[lsb] = pair.textures->at(i);
								instances[instance_resolved + instance_to_resolve]->SetTextureSlot(i, lsb);
								--msb;
							}
						}
					}

					if (reserved_texture_tolerant)
					{
						for (size_t i = 0; i < pair.reservedTextures->size(); ++i)
						{
							// should be tolerant to the one reserved texture per each.
							if (pair.reservedTextures->at(i) && reserved_textures[i] != nullptr)
							{
								reserved_textures[i] = pair.reservedTextures->at(i);
							}
						}
					}
					
					instance_to_resolve++;
				}
			}

			instance_buffer.SetDataPointerContainer(context, static_cast<UINT>(instance_to_resolve), instances.data() + instance_resolved);
			instance_buffer.TransitionToSRV(context);
			instance_buffer.CopySRVHeap(context);

			const Strong<Resources::Mesh>& locked_mesh = mesh.lock();
			
			for (size_t i = 0; i < instance_to_resolve; ++i)
			{
				for (size_t j = 0; j < assigned_texture.size(); ++j)
				{
					if (const Strong<Resources::Texture>& tex = assigned_texture[j])
					{
						RecordUsedTexture(context, gi, tex);
						gi.Bind(context, tex.get(), BIND_TYPE_SRV, BIND_SLOT_TEX, j);
					}
				}
				
				if (texture_pairs[instance_resolved + i].reservedTextures->size())
				{
					for (size_t j = 0; j < texture_pairs[instance_resolved + i].reservedTextures->size(); ++j)
					{
						if (const Strong<Resources::Texture>& tex = texture_pairs[instance_resolved + i].reservedTextures->at(j))
						{
							if (tex->GetTypeHash() == Resources::AtlasAnimationTexture::StaticTypeHash())
							{
								RecordUsedTexture(context, gi, tex);
								gi.Bind(context, tex.get(), BIND_TYPE_SRV, RESERVED_USER_TEX_ATLAS, 0);
							}
							else if (tex->GetTypeHash() == Resources::AnimationTexture::StaticTypeHash())
							{
								RecordUsedTexture(context, gi, tex);
								gi.Bind(context, tex.get(), BIND_TYPE_SRV, RESERVED_USER_TEX_BONES, 0);
							}
						}
					}
				}
			}

			gi.Draw(context, locked_mesh.get(), instance_to_resolve, instance_resolved);

			instance_buffer.TransitionCommon(context);
			instance_resolved += instance_to_resolve;
		}
	}
}

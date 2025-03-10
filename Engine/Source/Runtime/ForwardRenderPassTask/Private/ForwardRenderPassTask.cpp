#include "ForwardRenderPassTask.h"
#include "ForwardRenderPassTask.generated.h"

#include <ranges>
#include <tbb/parallel_for_each.h>

#include "RenderPipeline.h"
#include "Renderer.h"

#include "AtlasAnimationTexture.h"
#include "Material.h"
#include "Shape.h"

namespace Engine
{
	ForwardRenderPassTask::ForwardRenderPassTask()
		: m_gi_ticket_(SingletonSpinLock::GetInstance().Register()),
		  m_local_param_pool_ticket_(SingletonSpinLock::GetInstance().Register()),
		  m_instance_pool_ticket_(SingletonSpinLock::GetInstance().Register()),
		  m_texture_record_ticket_(SingletonSpinLock::GetInstance().Register()) {}

	void ForwardRenderPassTask::Run(
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
		if (domain_map->empty())
		{
			return;
		}
		
		// Filter the instances by the predicate
		uint64_t instance_count = 0;
        IntermediateShaderMap intermediate_shader_map;
        PredicateObject( predicate, domain_map, instance_count, intermediate_shader_map );

		m_local_param_pool_.Update(nullptr, instance_count);
		m_instance_pool_.Update(nullptr, instance_count);

		for (const auto& renderer : *domain_map | std::views::values)
		{
            for ( const auto &[ shader, mesh_map ] : renderer )
			{
                tbb::parallel_for_each(
                        mesh_map.begin(),
                        mesh_map.end(),
                        [ this,
                          shader,
                          &local_param,
                          &postrender_predicates,
                          &prerender_predicates,
                          &prerender_predicate,
                          &postrender_predicate,
                          &additional_sbs,
                          &shader_bypass,
                          &dt,
                          &intermediate_shader_map ](
                                const std::pair<Strong<Resources::Mesh>, aligned_vector<InstancePair>> &pair )
                        {
                            if ( const Strong<Resources::Shader> &locked = Cast<Resources::Shader>( shader ) )
                            {
                                if ( decltype( intermediate_shader_map )::const_accessor acc;
                                     intermediate_shader_map.find( acc, shader.get() ) )
                                {
                                    StartPhase_MultiThread( dt,
                                                            shader_bypass,
                                                            locked.get(),
                                                            pair.first.get(),
                                                            additional_sbs,
                                                            local_param,
                                                            prerender_predicate,
                                                            postrender_predicate,
                                                            prerender_predicates,
                                                            postrender_predicates,
                                                            acc->second );
                                }
                            }
                        } );
			}
		}

		auto& gi = g_graphic_accessor.GetInterface();
		auto context = gi.GetNewContext(0, false, L"Lazy Shader Resource Texture Transition Back");
		auto primitive = context.GetPointers();
		
		primitive.commandList->SoftReset();
        const auto  &range         = std::ranges::unique( m_used_shader_textures_ );
        const size_t indeterminate = std::distance( range.begin(), range.end() );
        const size_t unique_idx    = m_used_shader_textures_.size() - indeterminate -
                                  ( ( m_used_shader_textures_.size() > 0 && indeterminate == 0 &&
                                      m_used_shader_textures_.back() == nullptr ) ?
                                            1 :
                                            0 );
        if ( m_used_shader_textures_.size() > 0 && m_used_shader_textures_[ 0 ] != nullptr )
        {
            gi.TransitBackMultiple( &primitive, m_used_shader_textures_.data(), unique_idx, BIND_TYPE_SRV );
            std::ranges::fill( m_used_shader_textures_, nullptr );
        }
        primitive.commandList->FlagReady();
	}

	void ForwardRenderPassTask::Cleanup()
	{
		m_local_param_pool_.reset();
		m_instance_pool_.reset();
		m_heaps_.clear();
		std::ranges::fill(m_used_shader_textures_, nullptr);
	}

	void ForwardRenderPassTask::PredicateObject( const ObjectPredication &predicate,
                                                 RenderMap const         *domain_map,
                                                 uint64_t                &instance_count,
                                                 IntermediateShaderMap   &out_map ) const
    {
		out_map.clear();
		
		for (const auto& shaders : *domain_map | std::views::values)
		{
			tbb::parallel_for_each
			(
				shaders.begin(), shaders.end(), [&](const std::pair<Strong<Resources::ShaderBase>, MeshMap>& shader_pair)
				{
                    for ( const aligned_vector<InstancePair> &instances : shader_pair.second | std::views::values )
					{
						for (const InstancePair& instance_pair : instances)
						{
							if (predicate && !predicate(instance_pair.object))
							{
								continue;
							}

							std::remove_reference_t<decltype(out_map)>::accessor acc;
                            if ( !out_map.find( acc, shader_pair.first.get() ) )
							{
                                out_map.insert( acc, shader_pair.first.get() );
							}

							acc->second.push_back(instance_pair);
							++instance_count;
						}
					}
				}
			);
		}
	}

	void ForwardRenderPassTask::StartPhase_MultiThread(
            const float                                                       dt,
            const bool                                                        shader_bypass,
            const Resources::Shader                                          *shader,
            const Resources::Mesh                                            *mesh,
            const aligned_vector<const StructuredBufferDecorator *>          &additional_sbs,
            const Graphics::SBs::LocalParamSB                                &local_param,
            const ContextSetupFunction                                       &prerender_predicate,
            const ContextSetupFunction                                       &postrender_predicate,
            const std::unordered_map<std::string_view, ContextSetupFunction> &prerender_predicates,
            const std::unordered_map<std::string_view, ContextSetupFunction> &postrender_predicates,
            const aligned_vector<InstancePair>                               &instance_pairs
	)
	{
		IGraphicAPI& gi = g_graphic_accessor.GetInterface();

		// Manual release
		SpinLockToken gi_token = SingletonSpinLock::GetInstance().Lock(m_gi_ticket_);
		const IGraphicContextImpl& context = std::move(gi.GetNewContext(0, false, L"Render Pass"));
		gi_token.Release();
		
		const IGraphicContext&  primitive = context.GetPointers();
		primitive.commandList->SoftReset();

		// Manual release
		SpinLockToken local_param_token = SingletonSpinLock::GetInstance().Lock(m_local_param_pool_ticket_);
		m_local_param_pool_.advance();
		StructuredBufferTypeProxy<Graphics::SBs::LocalParamSB>& sb = m_local_param_pool_.get();
		local_param_token.Release();

		IHeapBase*                       current_heap = m_heaps_.emplace_back(gi.GetHeap())->get();
		const IGraphicContext temp_context
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
		auto instance_token = SingletonSpinLock::GetInstance().Lock(m_instance_pool_ticket_);
		m_instance_pool_.advance();
		StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>& instance = m_instance_pool_.get();
		instance_token.Release();
		
		aligned_vector<Graphics::SBs::InstanceSB *> instances;
        aligned_vector<TexturePair>                 texture_pairs;

        if ( instance_pairs.size() > instances.size() )
        {
            instances.resize( instance_pairs.size() );
            texture_pairs.resize( instance_pairs.size() );
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

	void ForwardRenderPassTask::RecordUsedTexture(
		const IGraphicContext* context, IGraphicAPI& gi, const Resources::Texture* tex
	)
	{
		auto tt = SingletonSpinLock::GetInstance().Lock(m_texture_record_ticket_);
		if (std::ranges::find(m_used_shader_textures_, tex) == m_used_shader_textures_.end())
		{
			gi.TransitTo(context, tex, BIND_TYPE_SRV);
			if (const auto& empty_slot = std::ranges::find(m_used_shader_textures_, nullptr);
				empty_slot == m_used_shader_textures_.end())
			{
				m_used_shader_textures_.push_back(tex);	
			}
			else
			{
				*empty_slot = tex;
			}
		}
	}

	void ForwardRenderPassTask::DrawPhase_MultiThread( 
		float                                                 dt,
		bool                                                  shader_bypass,
		size_t                                                instance_count,
		StructuredBufferTypeProxy<Graphics::SBs::InstanceSB> &instance_buffer,
		const Resources::Shader                              *shader,
		const Resources::Mesh                                *mesh,
		const IGraphicContext               *context,
		const aligned_vector<Graphics::SBs::InstanceSB *>    &instances,
		const aligned_vector<TexturePair>                    &texture_pairs
	)
	{
		CheckSize<UINT>(instance_count, L"Warning: Renderer will take a lot of amount of instance buffers!");

		// Manual release
		auto token = SingletonSpinLock::GetInstance().Lock(m_gi_ticket_);
		IGraphicAPI& gi = g_graphic_accessor.GetInterface();
		token.Release();

		if (!shader_bypass)
		{
            gi.BindGraphic( context, shader );
		}

		size_t instance_resolved = 0;
		while (instance_resolved != instance_count)
		{
			size_t instance_to_resolve = 0;

			constexpr size_t max_tex_binds = BIND_SLOT_TEXARR - BIND_SLOT_TEX;
			uint16_t tex_bind_mask = 0; // See max tex binds
            Resources::Texture *assigned_texture[ std::numeric_limits<uint16_t>::digits ]{};
            Resources::Texture *reserved_textures[ RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN ]{};
			
			while (const uint16_t count = _tzcnt_u16(tex_bind_mask))
			{
				if (instance_resolved + instance_to_resolve == instance_count)
				{
					break;
				}
				
				const TexturePair& pair = texture_pairs[ instance_resolved + instance_to_resolve ];
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
						{
                            // allow to instance with the first reserved texture encountered.
                            reserved_texture_tolerant = true;
                            reserved_textures[ i ]    = pair.reservedTextures->at( i ).get();
						}
					}
                    else if ( pair.reservedTextures->at( i ).get() == reserved_textures[ i ] &&
                              reserved_textures[ i ] != nullptr )
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
				
				if (count > pair.GetTextureCount())
				{
					size_t msb = count - 1;
					size_t lsb = max_tex_binds - count;
					for (size_t i = 0; i < pair.GetTextureCount(); ++i)
					{
						if (pair.textures->at(i))
						{
							if (const auto& it = std::ranges::find( assigned_texture, pair.textures->at( i ).get() );
								it != std::end( assigned_texture ) )
							{
                                const size_t bind_slot = std::distance( std::begin( assigned_texture ), it );
                                instances[ instance_resolved + instance_to_resolve ]->SetTextureSlot( i, bind_slot );
							}
							else
							{
                                tex_bind_mask |= 1 << msb;
                                assigned_texture[ lsb ] = pair.textures->at( i ).get();
                                instances[ instance_resolved + instance_to_resolve ]->SetTextureSlot( i, lsb );
                                --msb;
                                ++lsb;
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
								reserved_textures[i] = pair.reservedTextures->at(i).get();
							}
						}
					}
					
					instance_to_resolve++;
				}
			}

			instance_buffer.SetDataPointerContainer(context, static_cast<UINT>(instance_to_resolve), instances.data() + instance_resolved);
			instance_buffer.TransitionToSRV(context);
			instance_buffer.CopySRVHeap(context);
			
			for ( size_t j = 0; j < std::size( assigned_texture ); ++j )
            {
                if ( const Resources::Texture *const &tex = assigned_texture[ j ] )
                {
                    RecordUsedTexture( context, gi, tex );
                    gi.Bind( context, tex, BIND_TYPE_SRV, BIND_SLOT_TEX, j );
                }
            }

			for ( size_t i = 0; i < instance_to_resolve; ++i )
            {
                if ( texture_pairs[ instance_resolved + i ].reservedTextures->size() )
                {
                    for ( size_t j = 0; j < texture_pairs[ instance_resolved + i ].reservedTextures->size(); ++j )
                    {
                        if ( const Strong<Resources::Texture> &tex =
                                     texture_pairs[ instance_resolved + i ].reservedTextures->at( j ) )
                        {
                            if ( tex->GetTypeHash() == Resources::AtlasAnimationTexture::StaticTypeHash() )
                            {
                                RecordUsedTexture( context, gi, tex.get() );
                                gi.Bind( context, tex.get(), BIND_TYPE_SRV, RESERVED_USER_TEX_ATLAS, 0 );
                            }
                            else if ( tex->GetTypeHash() == Resources::AnimationTexture::StaticTypeHash() )
                            {
                                RecordUsedTexture( context, gi, tex.get() );
                                gi.Bind( context, tex.get(), BIND_TYPE_SRV, RESERVED_USER_TEX_BONES, 0 );
                            }
                        }
                    }
                }
            }

			gi.Draw( context, mesh, instance_to_resolve, instance_resolved );
			
			instance_buffer.TransitionCommon(context);
			instance_resolved += instance_to_resolve;
		}
	}

    void ForwardRenderPassTask::PreRun( const RenderMap *render_map,
            const size_t render_map_count,
            const ObjectPredication &predication)
    {}
}

#include "RaytracingRenderPassTask.h"

#include <ranges>
#include <vector>
#include <tbb/parallel_for_each.h>

#include "IRaytracingExtension.h"
#include "RenderPipeline.h"
#include "Renderer.h"

#include "Material.h"
#include "Shape.h"
#include "RaytracingShader.h"

namespace Engine
{
	RaytracingRenderPassTask::RaytracingRenderPassTask()
        : m_gi_ticket_(SingletonSpinLock::GetInstance().Register()),
          m_local_param_pool_ticket_(SingletonSpinLock::GetInstance().Register()),
          m_instance_pool_ticket_(SingletonSpinLock::GetInstance().Register()),
          m_texture_record_ticket_(SingletonSpinLock::GetInstance().Register()),
          m_heap_ticket_(SingletonSpinLock::GetInstance().Register()),
          m_byte_stream_ticket_(SingletonSpinLock::GetInstance().Register()) {}

    void RaytracingRenderPassTask::PreRun(RenderMap const* render_map, const size_t render_map_count, const ObjectPredication& predication)
	{
	    IGraphicAPI& gi = g_graphic_accessor.GetInterface();

        if ( IRaytracingExtension& rgi = g_graphic_accessor.GetRaytracingInterface();
             rgi.ShouldUseRaytracing() )
	    {
	        const auto& context = gi.GetNewContext(0, false, L"Building Top Level Acceleration Buffer");
	        const auto& primitive = context.GetPointers();
	        primitive.commandList->SoftReset();
	    
	        rgi.BuildTopLevelAccelerationBuffer(
                &primitive,
                render_map,
                render_map_count,
                m_top_level_acceleration_buffer_,
                predication);
	    
	        primitive.commandList->FlagReady();   
	    }
	}

	void RaytracingRenderPassTask::Run(
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

        if ( IRaytracingExtension& rgi = g_graphic_accessor.GetRaytracingInterface();
             rgi.ShouldUseRaytracing() )
		{
		    // Filter the instances by the predicate
		    uint64_t instance_count = 0;
		    IntermediateShaderMap intermediate_shader_map;
		    PredicateObject(predicate, domain_map, instance_count, intermediate_shader_map);

		    m_local_param_pool_.Update(nullptr, instance_count);
		    m_instance_pool_.Update(nullptr, instance_count);

            for (const auto& renderer : *domain_map | std::views::values)
            {
                tbb::parallel_for_each
                    (
                     renderer.begin(), renderer.end(),
                     [this, &intermediate_shader_map, &dt, &shader_bypass, &additional_sbs, &local_param,
                         &prerender_predicate, &postrender_predicate, &prerender_predicates, &postrender_predicates]
                         (const std::pair<Strong<Resources::ShaderBase>, MeshMap>& pair)
                     {
                         std::vector<RaytracingMeshInstancePair> meshes;
                         meshes.reserve(pair.second.size());
                         for (const auto& [mesh, instances] : pair.second)
                         {
                             meshes.emplace_back(static_cast<Resources::Mesh*>(mesh.get()), instances.size());
                         }

                         if (const Strong<Resources::RaytracingShader>& locked_shader =
                             Cast<Resources::RaytracingShader>(pair.first))
                         {
                             if (decltype(intermediate_shader_map)::const_accessor acc;
                             intermediate_shader_map.find(acc, locked_shader.get()))
                             {
                                 StartPhase_MultiThread(
                                      dt, shader_bypass, meshes, locked_shader.get(), additional_sbs, local_param,
                                      prerender_predicate, postrender_predicate, prerender_predicates, postrender_predicates,
                                      acc->second);
                             }
                         }
                     }
                    );
            }

		    auto& gi = g_graphic_accessor.GetInterface();
		    auto context = gi.GetNewContext(0, false, L"Lazy Shader Resource Texture Transition Back");
		    auto primitive = context.GetPointers();
		    
			primitive.commandList->SoftReset();
            const auto&  range         = std::ranges::unique( m_used_shader_textures_ );
            const size_t indeterminate = std::distance( range.begin(), range.end() );
            const size_t unique_idx    = m_used_shader_textures_.size() - indeterminate -
                                      ( ( m_used_shader_textures_.size() > 0 && indeterminate == 0 &&
                                          m_used_shader_textures_.back() == nullptr ) ?
                                                1 :
                                                0 );
		    if (m_used_shader_textures_.size() > 0 && m_used_shader_textures_[0] != nullptr)
		    {
			    std::vector<const Abstracts::Resource*> res_ptrs( m_used_shader_textures_.begin(), m_used_shader_textures_.begin() + unique_idx );
			    gi.TransitBackMultiple( &primitive, res_ptrs.data(), unique_idx, BIND_TYPE_SRV );
                std::ranges::fill( m_used_shader_textures_, nullptr );
		    }
            rgi.CopyRaytracingToRenderTarget( &primitive );
		    primitive.commandList->FlagReady();
		}
	}

	void RaytracingRenderPassTask::Cleanup()
	{
        if ( IRaytracingExtension& rgi = g_graphic_accessor.GetRaytracingInterface();
             rgi.ShouldUseRaytracing() )
	    {
	        m_local_param_pool_.reset();
	        m_instance_pool_.reset();
	        m_local_heaps_.clear();

	        for (auto& mask : m_byte_stream_usage_)
	        {
	            mask = 0;
	        }

	        for (auto& stream : m_byte_stream_)
	        {
	            stream.reset();
	        }
	    
	        std::ranges::fill(m_used_shader_textures_, nullptr);
	    }
	}

	void RaytracingRenderPassTask::PredicateObject(const ObjectPredication& predicate, RenderMap const* domain_map, uint64_t& instance_count, IntermediateShaderMap& out_map) const
	{
		out_map.clear();
		
		for (const auto& meshes : *domain_map | std::views::values)
		{
			tbb::parallel_for_each
			(
				meshes.begin(), meshes.end(), [&](const std::pair<Strong<Resources::ShaderBase>, MeshMap>& shader_pair)
				{
					for (const aligned_vector<InstancePair>& instances : shader_pair.second | std::views::values)
					{
						for (const InstancePair& instance_pair : instances)
						{
							if (predicate && !predicate(instance_pair.object))
							{
								continue;
							}

							std::remove_reference_t<decltype(out_map)>::accessor acc;
							if (!out_map.find(acc, shader_pair.first.get()))
							{
								out_map.insert(acc, shader_pair.first.get());
							}

							acc->second.push_back(instance_pair);
							++instance_count;
						}
					}
				}
			);
		}
	}

    void RaytracingRenderPassTask::StartPhase_MultiThread(
		const float                                                       dt,
		const bool                                                        shader_bypass,
		const std::vector<RaytracingMeshInstancePair>&                    meshes,
		const Resources::RaytracingShader*                                shader,
		const aligned_vector<const StructuredBufferDecorator*>&           additional_sbs,
		const Graphics::SBs::LocalParamSB&                                local_param,
		const ContextSetupFunction&                                       prerender_predicate,
		const ContextSetupFunction&                                       postrender_predicate,
		const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_predicates,
		const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_predicates,
		const aligned_vector<InstancePair>&                               instance_pairs
	)
	{
        IGraphicAPI& gi = g_graphic_accessor.GetInterface();
	    IRaytracingExtension& rgi = g_graphic_accessor.GetRaytracingInterface();

        // Manual release
        SpinLockToken                            gi_token     = SingletonSpinLock::GetInstance().Lock(m_gi_ticket_);
        const IGraphicContextImpl& context      = gi.GetNewContext(0, false, L"Raytracing Render Pass");
	    gi_token.Release();
		
		const IGraphicContext& primitive = context.GetPointers();
		primitive.commandList->SoftReset();

		// Manual release
		SpinLockToken local_param_token = SingletonSpinLock::GetInstance().Lock(m_local_param_pool_ticket_);
		m_local_param_pool_.advance();
		StructuredBufferTypeProxy<Graphics::SBs::LocalParamSB>& sb = m_local_param_pool_.get();
		local_param_token.Release();

		const IGraphicContext temp_context
		{
			.commandList = primitive.commandList,
			.heap = nullptr
		};

		sb.SetData(&temp_context, 1, &local_param);
		sb.TransitionToSRV(&temp_context);
	    
		gi.SetDefaultRenderTarget(&temp_context);
		gi.SetViewport(&temp_context, Managers::RenderPipeline::GetInstance().GetViewport());

		if (prerender_predicate) { prerender_predicate(&temp_context); }

		for ( const auto& func : prerender_predicates | std::views::values )
		{
			func( &temp_context );
		}
		
		// Manual release
		auto instance_token = SingletonSpinLock::GetInstance().Lock( m_instance_pool_ticket_ );
		m_instance_pool_.advance();
		StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>& instance = m_instance_pool_.get();
		instance_token.Release();
		
		static aligned_vector<Graphics::SBs::InstanceSB*> instances;
		
		if ( instance_pairs.size() > instances.capacity() )
		{
			instances.resize( instance_pairs.size() );
		}

		size_t idx = 0;
		for ( const InstancePair& instance_pair : instance_pairs )
		{
			instances[ idx ] = instance_pair.instance;
			++idx;
		}

		for ( const StructuredBufferDecorator* additional_sb : additional_sbs )
		{
			additional_sb->TransitionToSRV(&temp_context);
		}
	    
	    byte_stream& hit_records = GetByteStream();
	    size_t instance_pos = 0;
	    for ( const auto& [ mesh, instance_count ] : meshes )
	    {
	        for ( size_t i = 0; i < instance_count; ++i )
	        {
	            HitShaderRecord record{};

	            // Manual Release
	            auto token = SingletonSpinLock::GetInstance().Lock(m_heap_ticket_);
	            m_local_heaps_.push_back( rgi.GetRaytracingHeap() );
	            IHeapBase* heap = m_local_heaps_.back().get();
	            token.Release();

	            const IGraphicContext local_sig_context
                    {
                        .commandList = primitive.commandList,
                        .heap = heap
                    };
	                
	            for ( size_t j = 0; j < instance_pairs[i].textures.size(); ++j )
	            {
	                if ( instance_pairs[instance_pos + i].textures[j] )
	                {
	                    RecordUsedTexture( &local_sig_context, gi, instance_pairs[instance_pos + i].textures[j] );
	                    instance_pairs[instance_pos + i].instance->SetTextureSlot( j, 0 );
	                    gi.Bind( &local_sig_context, instance_pairs[instance_pos + i].textures[j].get(), BIND_TYPE_SRV, BIND_SLOT_TEX, j );
	                }
	            }

	            sb.CopySRVHeap( &local_sig_context );
	                
	            for ( const StructuredBufferDecorator* additional_sb : additional_sbs )
	            {
	                additional_sb->CopySRVHeap( &local_sig_context );
	            }

	            heap->SetSampler( shader, SAMPLER_TEXTURE );

	            record.addresses[ RAYTRACING_LOCAL_SLOT_SRV ] = heap->GetBufferHeapGPUAddress( g_local_raytracing_srv_offset );
	            record.addresses[ RAYTRACING_LOCAL_SLOT_UAV ] = heap->GetBufferHeapGPUAddress( g_local_raytracing_uav_offset );
	            record.addresses[ RAYTRACING_LOCAL_SLOT_SAMPLER ] = heap->GetSamplerHeapGPUAddress(0);
	            
	            record.addresses[ RAYTRACING_LOCAL_SLOT_VERTEX ] = mesh->GetVertexStructuredBuffer().GetGPUAddress();
	            record.addresses[ RAYTRACING_LOCAL_SLOT_INDEX ] = mesh->GetPrimitive()->GetNativeIndexBufferGPUAddress();
	            
	            hit_records.push_back(&record, sizeof(HitShaderRecord));
	        }

	        instance_pos += instance_count;
	    }
		
		DispatchPhase_MultiThread( dt, shader_bypass, idx, instance, shader, hit_records, &temp_context, instances );

		if (postrender_predicate) { postrender_predicate( &temp_context ); }

		for ( const auto& func : postrender_predicates | std::views::values )
		{
			func( &temp_context );
		}
		
		for ( const StructuredBufferDecorator* additional_sb : additional_sbs )
		{
			additional_sb->TransitionCommon( &temp_context );
		}

		sb.TransitionCommon( &temp_context );
		temp_context.commandList->FlagReady();
	}

	void RaytracingRenderPassTask::RecordUsedTexture(
		const IGraphicContext* context, IGraphicAPI& gi, const Strong<Resources::Texture>& tex
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

    byte_stream& RaytracingRenderPassTask::GetByteStream()
	{
	    const auto& token = SingletonSpinLock::GetInstance().Lock(m_byte_stream_ticket_);
	    for (size_t i = 0; i < m_byte_stream_usage_.size(); ++i)
	    {
	        if (const auto& trail = _tzcnt_u32(m_byte_stream_usage_[i]))
	        {
	            const auto& offset = i * std::numeric_limits<uint32_t>::digits;
	            m_byte_stream_usage_[i] |= 1 << trail;
	            return m_byte_stream_[offset + (std::numeric_limits<uint32_t>::digits - trail)];
	        }
	    }
	    
	    m_byte_stream_.resize( m_byte_stream_.size() + std::numeric_limits<uint32_t>::digits );
	    m_byte_stream_usage_.insert(m_byte_stream_usage_.end(), 1, 0);
	    m_byte_stream_usage_.back() |= 1 << (std::numeric_limits<uint32_t>::digits - 1);
	    return m_byte_stream_[(m_byte_stream_usage_.size() - 1) * std::numeric_limits<uint32_t>::digits];
	}

    void RaytracingRenderPassTask::DispatchPhase_MultiThread(
		float                                                 dt,
		bool                                                  shader_bypass,
		const size_t                                          instance_count,
		StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>& instance_buffer,
		const Resources::RaytracingShader*                    shader,
		const byte_stream&                                    hit_records,
		const IGraphicContext*               context,
		const aligned_vector<Graphics::SBs::InstanceSB*>&     instances)
	{
		CheckSize<UINT>(instance_count, L"Warning: Renderer will take a lot of amount of instance buffers!");
		IRaytracingExtension& rgi = g_graphic_accessor.GetRaytracingInterface();

	    instance_buffer.SetDataPointerContainer(context, static_cast<UINT>(instance_count), instances.data());
	    instance_buffer.TransitionToSRV(context);
	    
		rgi.DispatchRay
		    (
             context,
             shader_bypass ? nullptr : shader,
             Managers::RenderPipeline::GetInstance().GetLightSB(),
             instance_buffer,
             Managers::RenderPipeline::GetInstance().GetPerspectiveCB(),
             Managers::RenderPipeline::GetInstance().GetParamCB(),
             hit_records,
             m_top_level_acceleration_buffer_
		    );

		instance_buffer.TransitionCommon(context);
	}
}

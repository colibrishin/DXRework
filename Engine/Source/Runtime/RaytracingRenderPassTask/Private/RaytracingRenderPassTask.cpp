#include "RaytracingRenderPassTask.h"

#include <ranges>
#include <array>
#include <unordered_map>
#include <vector>
#include <tbb/parallel_for_each.h>

#include "IRaytracingExtension.h"
#include "RenderPipeline.h"
#include "Renderer.h"

#include "Material.h"
#include "Shape.h"
#include "RaytracingShader.h"

namespace
{
    // Texture pointers of one instance: instances with the same set share a hit group descriptor table.
    using RaytracingTextureSet = std::array<const Engine::Abstracts::Resource*, Engine::g_max_texture_per_material>;

    struct RaytracingTextureSetHash
    {
        size_t operator()(const RaytracingTextureSet& set) const noexcept
        {
            size_t h = 0;
            for (const Engine::Abstracts::Resource* res : set)
            {
                h = h * 1000003u ^ std::hash<const void*>{}(res);
            }
            return h;
        }
    };
}

namespace Engine
{
	RaytracingRenderPassTask::RaytracingRenderPassTask()
        : m_gi_ticket_(SingletonSpinLock::GetInstance().Register()),
          m_instance_pool_ticket_(SingletonSpinLock::GetInstance().Register()),
          m_texture_record_ticket_(SingletonSpinLock::GetInstance().Register()),
          m_heap_ticket_(SingletonSpinLock::GetInstance().Register()),
          m_byte_stream_ticket_(SingletonSpinLock::GetInstance().Register()) {}

    void RaytracingRenderPassTask::PreRun(RenderMap const* render_map, const size_t render_map_count, const ObjectPredication& predication)
	{
	    // The top level structure is built in Run, from the same instance list as the hit records and instance buffer.
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
		    // One list feeds the top level structure, the hit records and the instance buffer, so TLAS entry i, hit
		    // record i and instance i are the same object (the shader reads g_Instance[InstanceIndex()] and the hit
		    // group index is i): instances of the ray tracing shader whose mesh has a bottom level structure, in mesh
		    // order. Objects of other shaders are not traced.
		    Strong<Resources::RaytracingShader>     shader;
		    std::vector<RaytracingMeshInstancePair> meshes;
		    std::vector<const InstancePair*>        instance_pairs; // into the render map, valid during Run
		    std::vector<RaytracingInstance>         tlas_instances;
		    size_t                                  upper_bound = 0;
		    for (const auto& renderer : *domain_map | std::views::values)
		    {
		        for (const auto& mesh_map : renderer | std::views::values)
		        {
		            for (const auto& instances : mesh_map | std::views::values)
		            {
		                upper_bound += instances.size();
		            }
		        }
		    }
		    instance_pairs.reserve(upper_bound);
		    tlas_instances.reserve(upper_bound);
		    for (const auto& renderer : *domain_map | std::views::values)
		    {
		        for (const auto& [shader_resource, mesh_map] : renderer)
		        {
		            const Strong<Resources::RaytracingShader> rt_shader = Cast<Resources::RaytracingShader>(shader_resource);
		            if (!rt_shader)
		            {
		                continue;
		            }
		            if (!shader)
		            {
		                shader = rt_shader;
		            }
		            else if (rt_shader != shader)
		            {
		                // One dispatch writes the whole output: a second shader would need its own hit groups.
		                static std::atomic<bool> s_warned{ false };
		                if (!s_warned.exchange(true))
		                {
		                    OutputDebugStringA("WARNING: ray tracing pass traces one ray tracing shader; others are skipped\n");
		                }
		                continue;
		            }

		            for (const auto& [mesh, instances] : mesh_map)
		            {
		                auto* const mesh_ptr = static_cast<Resources::Mesh*>(mesh.get());
		                if (mesh_ptr->GetBLAS().empty || !mesh_ptr->GetBLAS().resultPool || !mesh_ptr->GetPrimitive())
		                {
		                    static std::atomic<bool> s_warned{ false };
		                    if (!s_warned.exchange(true))
		                    {
		                        OutputDebugStringA("WARNING: ray tracing pass skips meshes without a bottom level structure\n");
		                    }
		                    continue;
		                }

		                size_t count = 0;
		                for (const InstancePair& instance_pair : instances)
		                {
		                    if (predicate && !predicate(instance_pair.object))
		                    {
		                        continue;
		                    }
		                    instance_pairs.push_back(&instance_pair); // no copy: an InstancePair is about 870 bytes
		                    tlas_instances.push_back({ mesh_ptr, instance_pair.instance });
		                    ++count;
		                }
		                if (count != 0)
		                {
		                    meshes.emplace_back(mesh_ptr, count);
		                }
		            }
		        }
		    }

		    if (instance_pairs.empty())
		    {
		        return;
		    }

		    // One hit record table per instance, from one shader-visible heap: past its size the rest is not traced.
		    if (const size_t reserved = rgi.ReserveRaytracingHeap(instance_pairs.size()); reserved < instance_pairs.size())
		    {
		        static std::atomic<bool> s_warned{ false };
		        if (!s_warned.exchange(true))
		        {
		            OutputDebugStringA("WARNING: ray tracing pass traces only the instances one descriptor heap holds\n");
		        }
		        instance_pairs.resize(reserved);
		        tlas_instances.resize(reserved);
		        size_t kept = 0;
		        for (auto it = meshes.begin(); it != meshes.end(); ++it)
		        {
		            if (kept + it->instanceCount >= reserved)
		            {
		                it->instanceCount = reserved - kept;
		                meshes.erase(it->instanceCount == 0 ? it : it + 1, meshes.end());
		                break;
		            }
		            kept += it->instanceCount;
		        }
		    }

		    {
		        IGraphicAPI& gi       = g_graphic_accessor.GetInterface();
		        const auto&  context  = gi.GetNewContext(0, false, L"Building Top Level Acceleration Buffer");
		        const auto&  primitive = context.GetPointers();
		        primitive.commandList->SoftReset();
		        rgi.BuildTopLevelAccelerationBuffer(&primitive, tlas_instances.data(), tlas_instances.size(),
		                                            m_top_level_acceleration_buffer_);
		        primitive.commandList->FlagReady();
		    }

		    StartPhase_MultiThread(
		         dt, shader_bypass, meshes, shader.get(), additional_sbs, local_param,
		         prerender_predicate, postrender_predicate, prerender_predicates, postrender_predicates, instance_pairs);

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
		const std::vector<const InstancePair*>&                           instance_pairs
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

		const IGraphicContext temp_context
		{
			.commandList = primitive.commandList,
			.heap = nullptr
		};

	    
		gi.SetDefaultRenderTarget(&temp_context);
		gi.SetViewport(&temp_context, Managers::RenderPipeline::GetInstance().GetViewport());

		// The pre/post render predicates bind raster resources (shadow maps, reflection map) into a heap; the ray
		// tracing pass has none and traces its own shadow and reflection rays, so it does not run them.
		
		// Manual release
		auto instance_token = SingletonSpinLock::GetInstance().Lock( m_instance_pool_ticket_ );
		m_instance_pool_.advance();
		StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>& instance = m_instance_pool_.get();
		instance_token.Release();
		
		aligned_vector<Graphics::SBs::InstanceSB*> instances( instance_pairs.size() );

		size_t idx = 0;
		for ( const InstancePair* instance_pair : instance_pairs )
		{
			instances[ idx ] = instance_pair->instance;
			++idx;
		}

		// The shared buffers (lights) are read without transitions, as in the raster passes, whose lists record in parallel:
		// they stay COMMON, are promoted implicitly on read and decay when their ExecuteCommandLists
		// finishes (this engine submits one list per call).
	    
	    std::unordered_map<RaytracingTextureSet, IHeapBase*, RaytracingTextureSetHash> tables;
	    byte_stream& hit_records = GetByteStream();
	    size_t instance_pos = 0;
	    for ( const auto& [ mesh, instance_count ] : meshes )
	    {
	        for ( size_t i = 0; i < instance_count; ++i )
	        {
	            HitShaderRecord record{};

	            const InstancePair& pair = *instance_pairs[instance_pos + i];

	            // Instances with the same textures share one descriptor table (the sampler table is shared by all):
	            // only the hit record, which holds the mesh buffers, is per instance.
	            RaytracingTextureSet key{};
	            for ( size_t j = 0; j < pair.textures.size(); ++j )
	            {
	                key[ j ] = pair.textures[ j ].get();
	            }

	            auto [ table, inserted ] = tables.try_emplace( key, nullptr );
	            if ( inserted )
	            {
	                // Manual Release
	                auto token = SingletonSpinLock::GetInstance().Lock(m_heap_ticket_);
	                m_local_heaps_.push_back( rgi.GetRaytracingHeap() );
	                table->second = m_local_heaps_.back().get();
	                token.Release();

	                const IGraphicContext local_sig_context
	                {
	                    .commandList = primitive.commandList,
	                    .heap = table->second
	                };

	                for ( size_t j = 0; j < pair.textures.size(); ++j )
	                {
	                    if ( pair.textures[j] )
	                    {
	                        RecordUsedTexture( &local_sig_context, gi, pair.textures[j] );
	                        gi.Bind( &local_sig_context, pair.textures[j].get(), BIND_TYPE_SRV, BIND_SLOT_TEX, j );
	                    }
	                }

	                // raytracing.hlsl reads only tex[] and PSSampler through the local tables (lights, instances and
	                // parameters come from the global root arguments), so the structured buffers are not copied here.
	                if ( tables.size() == 1 )
	                {
	                    table->second->SetSampler( shader, SAMPLER_TEXTURE );
	                }
	            }
	            IHeapBase* heap = table->second;

	            for ( size_t j = 0; j < pair.textures.size(); ++j )
	            {
	                if ( pair.textures[j] )
	                {
	                    pair.instance->SetTextureSlot( j, static_cast<int>( j ) ); // bound at BIND_SLOT_TEX + j
	                }
	            }

	            record.addresses[ RAYTRACING_LOCAL_SLOT_SRV ] = heap->GetBufferHeapGPUAddress( g_local_raytracing_srv_offset );
	            record.addresses[ RAYTRACING_LOCAL_SLOT_UAV ] = heap->GetBufferHeapGPUAddress( g_local_raytracing_uav_offset );
	            record.addresses[ RAYTRACING_LOCAL_SLOT_SAMPLER ] = heap->GetSamplerHeapGPUAddress(0);
	            
	            record.addresses[ RAYTRACING_LOCAL_SLOT_VERTEX ] = mesh->GetVertexStructuredBuffer().GetGPUAddress();
	            record.addresses[ RAYTRACING_LOCAL_SLOT_INDEX ] = mesh->GetPrimitive()->GetRaytracingIndexBufferGPUAddress();
	            
	            hit_records.push_back(&record, sizeof(HitShaderRecord));
	        }

	        instance_pos += instance_count;
	    }
		
		DispatchPhase_MultiThread( dt, shader_bypass, idx, instance, shader, hit_records, &temp_context, instances );

		

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

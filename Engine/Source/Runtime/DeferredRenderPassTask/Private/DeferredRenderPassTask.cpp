#include "DeferredRenderPassTask.h"

#include "Shader.h"
#include "ShaderBase.h"
#include "RenderPipeline.h"
#include "Mesh.h"
#include "AtlasAnimationTexture.h"
#include "AnimationTexture.h"

#include <vector>
#include <tbb/parallel_for_each.h>
#include "Texture2D.h"
#if CFG_RAYTRACING
#include "IRaytracingExtension.h"
#endif

Engine::DeferredRenderPassTask::DeferredRenderPassTask()
    : m_gi_ticket_( SingletonSpinLock::GetInstance().Register() ),
      m_local_param_pool_ticket_( SingletonSpinLock::GetInstance().Register() ),
      m_instance_pool_ticket_( SingletonSpinLock::GetInstance().Register() ),
      m_texture_record_ticket_( SingletonSpinLock::GetInstance().Register() )
{}

void Engine::DeferredRenderPassTask::Run(
        float                                                             dt,
        bool                                                              shader_bypass,
        RenderMap const                                                  *domain_map,
        const aligned_vector<const StructuredBufferDecorator *>          &additional_sbs,
        const Graphics::SBs::LocalParamSB                                &local_param,
        const ObjectPredication                                          &predicate,
        const ContextSetupFunction                                       &prerender_predicate,
        const ContextSetupFunction                                       &postrender_predicate,
        const std::unordered_map<std::string_view, ContextSetupFunction> &prerender_predicates,
        const std::unordered_map<std::string_view, ContextSetupFunction> &postrender_predicates )
{
    if ( domain_map->empty() )
    {
        return;
    }

    bool rt_ready = ( m_deferred_depth_raw_ != nullptr );
    for ( size_t i = 0; i < g_deferred_count; ++i )
    {
        rt_ready = rt_ready && ( m_deferred_render_targets_raw_[ i ] != nullptr );
    }
    if ( !rt_ready )
    {
        return;
    }

    IGraphicAPI &gi= g_graphic_accessor.GetInterface();
    {
        const auto &context   = gi.GetNewContext( 0, false, L"Clear Deferred Textures" );
        const auto &primitive = context.GetPointers();

        primitive.commandList->SoftReset();
        const Abstracts::Resource* rtv_ptrs[ g_deferred_count ];
        for ( size_t i = 0; i < g_deferred_count; ++i ) rtv_ptrs[ i ] = m_deferred_render_targets_raw_[ i ];
        gi.TransitToMultiple( &primitive, rtv_ptrs, g_deferred_count, BIND_TYPE_RTV );
#if CFG_RAYTRACING
        // Start from the ray traced depth (the clear stays if this frame was not traced): raster objects behind a ray
        // traced surface fail the depth test, so the light pass keeps the ray traced pixel there.
        if ( IRaytracingExtension& rgi = g_graphic_accessor.GetRaytracingInterface(); rgi.ShouldUseRaytracing() )
        {
            rgi.CopyRaytracingDepthTo( &primitive, m_deferred_depth_raw_ );
        }
#endif
        gi.TransitTo( &primitive, m_deferred_depth_raw_, BIND_TYPE_DSV );
        primitive.commandList->FlagReady();
    }
    
    // Filter the instances by the predicate
    uint64_t            instance_count = 0;
    IntermediateShaderMap intermediate_shader_map;
    PredicateObject( predicate, domain_map, instance_count, intermediate_shader_map );

    // The per-batch pools grow in advance(); sizing them to the instance count allocated one GPU buffer and one
    // descriptor heap per object (E_OUTOFMEMORY at ~5k objects).

    for ( const auto &renderer : *domain_map | std::views::values )
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
                            const MeshMap::value_type &pair )
                    {
                        if ( const Strong<Resources::Shader> &locked = Cast<Resources::Shader>( shader ) )
                        {
                            if ( decltype( intermediate_shader_map )::const_accessor acc;
                                 intermediate_shader_map.find( acc, &pair.second ) )
                            {
                                StartPhase_MultiThread( dt,
                                                        shader_bypass,
                                                        locked.get(),
                                                        static_cast<const Resources::Mesh*>( pair.first.get() ),
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

    LightPass( dt,
               shader_bypass,
               additional_sbs,
               local_param,
               prerender_predicate,
               postrender_predicate,
               prerender_predicates,
               postrender_predicates );

    {
        auto context   = gi.GetNewContext( 0, false, L"Lazy Shader Resource Texture Transition Back" );
        auto primitive = context.GetPointers();

		primitive.commandList->SoftReset();
        const auto&  range         = std::ranges::unique( m_used_shader_textures_ );
        const size_t indeterminate = std::distance( range.begin(), range.end() );
        const size_t unique_idx    = m_used_shader_textures_.size() - indeterminate -
                                  ( ( m_used_shader_textures_.size() > 0 && indeterminate == 0 &&
                                      m_used_shader_textures_.back() == nullptr ) ?
                                            1 :
                                            0 );
        if ( m_used_shader_textures_.size() > 0 && m_used_shader_textures_[ 0 ] != nullptr )
        {
            std::vector<const Abstracts::Resource*> res_ptrs( m_used_shader_textures_.begin(), m_used_shader_textures_.begin() + unique_idx );
            gi.TransitBackMultiple( &primitive, res_ptrs.data(), unique_idx, BIND_TYPE_SRV );
            std::ranges::fill( m_used_shader_textures_, nullptr );
        }
        gi.TransitBack( &primitive, m_deferred_depth_raw_, BIND_TYPE_DSV );
        primitive.commandList->FlagReady();
    }   
}

void Engine::DeferredRenderPassTask::Cleanup()
{
    if ( m_deferred_depth_raw_ == nullptr )
    {
        return;
    }
    IGraphicAPI &gi = g_graphic_accessor.GetInterface();
    const auto       &context = gi.GetNewContext( 0, false, L"Clear Deferred Textures" );
    const auto       &primitive = context.GetPointers();
    
    primitive.commandList->SoftReset();
    for ( size_t i = 0; i < std::size(m_deferred_render_targets_raw_); ++i )
    {
        if ( m_deferred_render_targets_raw_[ i ] != nullptr )
        {
            gi.Clear( &primitive, m_deferred_render_targets_raw_[i], BIND_TYPE_RTV );
        }
    }

    gi.Clear( &primitive, m_deferred_depth_raw_, BIND_TYPE_DSV );
    primitive.commandList->FlagReady();

    m_local_param_pool_.reset();
    m_instance_pool_.reset();
    m_heaps_.clear();
    std::ranges::fill( m_used_shader_textures_, nullptr );
}

void Engine::DeferredRenderPassTask::SetTexture( Resources::Texture2D* tex, const size_t slot )
{
    if ( g_deferred_count > slot )
    {
        m_deferred_render_targets_raw_[ slot ] = tex;
    }
}

void Engine::DeferredRenderPassTask::SetDepthStencil( Resources::Texture2D* tex )
{
    m_deferred_depth_raw_ = tex;
}

void Engine::DeferredRenderPassTask::SetLightShader( Resources::Shader* shader )
{
    m_light_pass_shader_raw_ = shader;
}

inline void Engine::DeferredRenderPassTask::PredicateObject( const ObjectPredication &predicate,
                                                             RenderMap const         *domain_map,
                                                             uint64_t                &instance_count,
                                                             IntermediateShaderMap   &out_map ) const
{
    out_map.clear();

    for ( const auto &shaders : *domain_map | std::views::values )
    {
        tbb::parallel_for_each( shaders.begin(),
                                shaders.end(),
                                [ & ]( const ShaderMap::value_type &shader_pair )
                                {
                                    for ( const aligned_vector<InstancePair> &instances :
                                          shader_pair.second | std::views::values )
                                    {
                                        // Filtered locally, then inserted once per instance list.
                                        aligned_vector<const InstancePair *> selected;
                                        selected.reserve( instances.size() );
                                        for ( const InstancePair &instance_pair : instances )
                                        {
                                            if ( predicate && !predicate( instance_pair.object ) )
                                            {
                                                continue;
                                            }
                                            selected.push_back( &instance_pair );
                                        }

                                        if ( selected.empty() )
                                        {
                                            continue;
                                        }

                                        std::atomic_ref( instance_count ).fetch_add( selected.size() );
                                        std::remove_reference_t<decltype( out_map )>::accessor acc;
                                        out_map.insert( acc, &instances );
                                        acc->second = std::move( selected );
                                    }
                                } );
    }
}

inline void Engine::DeferredRenderPassTask::StartPhase_MultiThread(
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
        const aligned_vector<const InstancePair *>                       &instance_pairs )
{
#if WITH_DEBUG
    const auto &shader_formats = shader->GetRTVFormat();
    if ( shader_formats.size() < g_deferred_count )
    {
        return;
    }

    for ( size_t i = 0; i < g_deferred_count; ++i)
    {
        if ( shader_formats[ i ] != g_deferred_format[ i ] )
        {
            return;
        }
    }
#endif

    IGraphicAPI &gi = g_graphic_accessor.GetInterface();

    // Manual release
    SpinLockToken                            gi_token = SingletonSpinLock::GetInstance().Lock( m_gi_ticket_ );
    const IGraphicContextImpl &context  = gi.GetNewContext( 0, false, L"Render Pass" ); // lifetime extended to the scope
    gi_token.Release();

    const IGraphicContext &primitive = context.GetPointers();
    primitive.commandList->SoftReset();

    // Manual release
    SpinLockToken local_param_token = SingletonSpinLock::GetInstance().Lock( m_local_param_pool_ticket_ );
    m_local_param_pool_.advance();
    StructuredBufferTypeProxy<Graphics::SBs::LocalParamSB> &sb = m_local_param_pool_.get();
    local_param_token.Release();

    IHeapBase                       *current_heap = m_heaps_.emplace_back( gi.GetHeap() )->get();
    const IGraphicContext temp_context{ .commandList = primitive.commandList, .heap = current_heap };

    current_heap->BindGraphic( &temp_context );
    sb.SetData( &temp_context, 1, &local_param );
    sb.TransitionToSRV( &temp_context );
    sb.CopySRVHeap( &temp_context );

    const Abstracts::Resource* rtv_ptrs[ g_deferred_count ];
    for ( size_t i = 0; i < g_deferred_count; ++i ) rtv_ptrs[ i ] = m_deferred_render_targets_raw_[ i ];
    gi.BindMultiple( &primitive, rtv_ptrs, g_deferred_count, m_deferred_depth_raw_ );
    Managers::RenderPipeline::GetInstance().BindConstantBuffers( &temp_context );
    gi.SetViewport( &temp_context, Managers::RenderPipeline::GetInstance().GetViewport() );

    if ( prerender_predicate )
    {
        prerender_predicate( &temp_context );
    }

    for ( const auto &func : prerender_predicates | std::views::values )
    {
        func( &temp_context );
    }

    // Manual release
    auto instance_token = SingletonSpinLock::GetInstance().Lock( m_instance_pool_ticket_ );
    m_instance_pool_.advance();
    StructuredBufferTypeProxy<Graphics::SBs::InstanceSB> &instance = m_instance_pool_.get();
    instance_token.Release();

    aligned_vector<Graphics::SBs::InstanceSB *> instances;
    aligned_vector<TexturePair>                 texture_pairs;

    if ( instance_pairs.size() > instances.size() )
    {
        instances.resize( instance_pairs.size() );
        texture_pairs.resize( instance_pairs.size() );
    }

    size_t idx = 0;
    for ( const InstancePair *instance_pair : instance_pairs )
    {
        instances[ idx ]     = instance_pair->instance;
        texture_pairs[ idx ] = TexturePair( &instance_pair->textures, &instance_pair->reservedTextures );
        texture_pairs[ idx ].instanceCount = instance_pair->instanceCount;
        ++idx;
    }

    // Shared buffers (lights, light view-projections) are read by lists recorded in parallel, so they are not transitioned
    // here: they stay COMMON, are promoted implicitly on read and decay when their ExecuteCommandLists
    // finishes (this engine submits one list per call). A tracked state shared
    // across lists raced and recorded barriers with matching states.
    for ( const StructuredBufferDecorator *additional_sb : additional_sbs )
    {
        additional_sb->CopySRVHeap( &temp_context );
    }

    // One batch per heap block: descriptors are written into the shader-visible block immediately, so a further batch
    // binding its textures and instance buffer into the same block changed what the earlier draws read, and its upload
    // overwrote the earlier batch's instance data before the GPU copied it. Each further batch gets its own block and
    // instance buffer, with the pass bindings repeated.
    static_assert( g_max_texture_per_material < BIND_SLOT_TEXARR - BIND_SLOT_TEX, "an empty batch must fit one material" );
    IGraphicContext                                       batch_context  = temp_context;
    StructuredBufferTypeProxy<Graphics::SBs::InstanceSB> *batch_instance = &instance;
    for ( size_t resolved = 0; resolved != idx; )
    {
        if ( resolved != 0 )
        {
            batch_context = IGraphicContext{ .commandList = primitive.commandList, .heap = m_heaps_.emplace_back( gi.GetHeap() )->get() };
            batch_context.heap->BindGraphic( &batch_context );
            sb.CopySRVHeap( &batch_context );
            Managers::RenderPipeline::GetInstance().BindConstantBuffers( &batch_context );
            if ( prerender_predicate )
            {
                prerender_predicate( &batch_context );
            }
            for ( const auto &func : prerender_predicates | std::views::values )
            {
                func( &batch_context );
            }
            for ( const StructuredBufferDecorator *additional_sb : additional_sbs )
            {
                additional_sb->CopySRVHeap( &batch_context );
            }

            auto batch_token = SingletonSpinLock::GetInstance().Lock( m_instance_pool_ticket_ );
            m_instance_pool_.advance();
            batch_instance = &m_instance_pool_.get();
        }

        const size_t drawn = MaterialPass_Multithread( dt, shader_bypass, idx, resolved, *batch_instance, shader, mesh, &batch_context, instances, texture_pairs );
        if ( drawn == 0 )
        {
            __fastfail( FAST_FAIL_INVALID_ARG ); // cannot happen (static_assert above); it would loop forever
        }
        resolved += drawn;
    }

    if ( postrender_predicate )
    {
        postrender_predicate( &temp_context );
    }

    for ( const auto &func : postrender_predicates | std::views::values )
    {
        func( &temp_context );
    }


    sb.TransitionCommon( &temp_context );
    temp_context.commandList->FlagReady();
}

inline size_t Engine::DeferredRenderPassTask::MaterialPass_Multithread(
        float                                                 dt,
        bool                                                  shader_bypass,
        size_t                                                instance_count,
        size_t                                                instance_resolved,
        StructuredBufferTypeProxy<Graphics::SBs::InstanceSB> &instance_buffer,
        const Resources::Shader                              *shader,
        const Resources::Mesh                                *mesh,
        const IGraphicContext               *context,
        const aligned_vector<Graphics::SBs::InstanceSB *>    &instances,
        const aligned_vector<TexturePair>                    &texture_pairs )
{
    CheckSize<UINT>( instance_count, L"Warning: Renderer will take a lot of amount of instance buffers!" );

    // Manual release
    auto              token = SingletonSpinLock::GetInstance().Lock( m_gi_ticket_ );
    IGraphicAPI &gi    = g_graphic_accessor.GetInterface();
    token.Release();

    if ( !shader_bypass )
    {
        gi.BindGraphic( context, shader );
    }

    {
        size_t instance_to_resolve = 0;

        constexpr size_t max_tex_binds = BIND_SLOT_TEXARR - BIND_SLOT_TEX;
        uint16_t         tex_bind_mask = 0; // See max tex binds
        Resources::Texture *assigned_texture[ std::numeric_limits<uint16_t>::digits ]{};
        Resources::Texture *reserved_textures[ RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN ]{};

        while ( const uint16_t count = _tzcnt_u16( tex_bind_mask ) )
        {
            if ( instance_resolved + instance_to_resolve == instance_count )
            {
                break;
            }

            const TexturePair &pair                      = texture_pairs[ instance_resolved + instance_to_resolve ];
            // A block of instances (InstancePair::instanceCount) is drawn by itself, from an empty batch.
            if ( pair.instanceCount != 0 && instance_to_resolve != 0 )
            {
                break;
            }
            const auto set_texture_slot = [ &, first = instances[ instance_resolved + instance_to_resolve ] ]( const size_t i, const size_t slot )
            {
                for ( size_t n = 0; n < std::max<size_t>( pair.instanceCount, 1 ); ++n )
                {
                    first[ n ].SetTextureSlot( i, static_cast<int>( slot ) );
                }
            };
            // Reserved textures (bones, atlas) bind one texture per slot for the whole batch: an instance
            // with a different one, or one whose textures no longer fit, starts the next batch.
            bool reserved_conflict = false;
            for ( size_t i = 0; i < pair.reservedTextures->size(); ++i )
            {
                const auto* reserved = static_cast<Resources::Texture*>( pair.reservedTextures->at( i ).get() );
                if ( reserved && reserved_textures[ i ] && reserved_textures[ i ] != reserved )
                {
                    reserved_conflict = true;
                    break;
                }
            }

            if ( reserved_conflict || count <= pair.GetTextureCount() )
            {
                break; // flush this batch (an empty batch always has room: 16 slots > 8 per material)
            }

            if ( count > pair.GetTextureCount() )
            {
                size_t msb = count - 1;
                size_t lsb = max_tex_binds - count;
                for ( size_t i = 0; i < pair.textures->size(); ++i ) // material slots may be sparse
                {
                    if ( pair.textures->at( i ) )
                    {
                        if ( const auto &it = std::ranges::find( assigned_texture, static_cast<Resources::Texture*>( pair.textures->at( i ).get() ) );
                             it != std::end( assigned_texture ) )
                        {
                            const size_t bind_slot = std::distance( std::begin( assigned_texture ), it );
                            set_texture_slot( i, bind_slot );
                        }
                        else
                        {
                            tex_bind_mask |= 1 << msb;
                            assigned_texture[ lsb ] = static_cast<Resources::Texture*>( pair.textures->at( i ).get() );
                            set_texture_slot( i, lsb );
                            --msb;
                            ++lsb;
                        }
                    }
                }

                for ( size_t i = 0; i < pair.reservedTextures->size(); ++i )
                {
                    if ( pair.reservedTextures->at( i ) && reserved_textures[ i ] == nullptr )
                    {
                        reserved_textures[ i ] = static_cast<Resources::Texture*>( pair.reservedTextures->at( i ).get() );
                    }
                }

                instance_to_resolve++;
                if ( pair.instanceCount != 0 )
                {
                    break; // the block fills the batch
                }
            }
        }

        // A block uploads its contiguous instances; otherwise gather the batch's instances.
        const size_t block = texture_pairs[ instance_resolved ].instanceCount;
        if ( block != 0 )
        {
            instance_buffer.SetData( context, static_cast<UINT>( block ), instances[ instance_resolved ] );
        }
        else
        {
            instance_buffer.SetDataPointerContainer( context, static_cast<UINT>( instance_to_resolve ), instances.data() + instance_resolved );
        }
        instance_buffer.TransitionToSRV( context );
        instance_buffer.CopySRVHeap( context );

        for ( size_t j = 0; j < std::size( assigned_texture ); ++j )
        {
            if ( const Resources::Texture* const& tex = assigned_texture[ j ] )
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
                    if ( const Strong<Abstracts::Resource> &raw =
                                 texture_pairs[ instance_resolved + i ].reservedTextures->at( j ) )
                    {
                        const auto* tex = static_cast<const Resources::Texture*>( raw.get() );
                        if ( tex->GetTypeHash() == Resources::AtlasAnimationTexture::StaticTypeHash() )
                        {
                            RecordUsedTexture( context, gi, tex );
                            gi.Bind( context, tex, BIND_TYPE_SRV, RESERVED_USER_TEX_ATLAS, 0 );
                        }
                        else if ( tex->GetTypeHash() == Resources::AnimationTexture::StaticTypeHash() )
                        {
                            RecordUsedTexture( context, gi, tex );
                            gi.Bind( context, tex, BIND_TYPE_SRV, RESERVED_USER_TEX_BONES, 0 );
                        }
                    }
                }
            }
        }

        gi.Draw( context, static_cast<const Abstracts::Resource*>( mesh ), static_cast<UINT>( block != 0 ? block : instance_to_resolve ), instance_resolved );

        instance_buffer.TransitionCommon( context );
        return instance_to_resolve;
    }
}

inline void Engine::DeferredRenderPassTask::LightPass(
        const float                                                       dt,
        const bool                                                        shader_bypass,
        const aligned_vector<const StructuredBufferDecorator *>          &additional_sbs,
        const Graphics::SBs::LocalParamSB                                &local_param,
        const ContextSetupFunction                                       &prerender_predicate,
        const ContextSetupFunction                                       &postrender_predicate,
        const std::unordered_map<std::string_view, ContextSetupFunction> &prerender_predicates,
        const std::unordered_map<std::string_view, ContextSetupFunction> &postrender_predicates )
{
    IGraphicAPI &gi = g_graphic_accessor.GetInterface();
    const IGraphicContextImpl &context  = gi.GetNewContext( 0, false, L"Render Pass" ); // lifetime extended to the scope
    const IGraphicContext &primitive = context.GetPointers();
    primitive.commandList->SoftReset();

    m_local_param_pool_.advance();
    StructuredBufferTypeProxy<Graphics::SBs::LocalParamSB> &sb = m_local_param_pool_.get();
    
    IHeapBase                       *current_heap = m_heaps_.emplace_back( gi.GetHeap() )->get();
    const IGraphicContext temp_context{ .commandList = primitive.commandList, .heap = current_heap };

    current_heap->BindGraphic( &temp_context );
    sb.SetData( &temp_context, 1, &local_param );
    sb.TransitionToSRV( &temp_context );
    sb.CopySRVHeap( &temp_context );

    const Abstracts::Resource* rtv_ptrs_lp[ g_deferred_count ];
    for ( size_t i = 0; i < g_deferred_count; ++i ) rtv_ptrs_lp[ i ] = m_deferred_render_targets_raw_[ i ];
    gi.TransitBackMultiple( &temp_context, rtv_ptrs_lp, g_deferred_count, BIND_TYPE_RTV );
    gi.TransitToMultiple( &temp_context, rtv_ptrs_lp, g_deferred_count, BIND_TYPE_SRV );
    gi.BindMultiple( &temp_context, rtv_ptrs_lp, BIND_TYPE_SRV, BIND_SLOT_TEX, 0, g_deferred_count );
    gi.SetDefaultRenderTarget( &temp_context );

    Managers::RenderPipeline::GetInstance().BindConstantBuffers( &temp_context );
    gi.SetViewport( &temp_context, Managers::RenderPipeline::GetInstance().GetViewport() );

    if ( prerender_predicate )
    {
        prerender_predicate( &temp_context );
    }

    for ( const auto &func : prerender_predicates | std::views::values )
    {
        func( &temp_context );
    }

    m_instance_pool_.advance();
    StructuredBufferTypeProxy<Graphics::SBs::InstanceSB> &instance = m_instance_pool_.get();

    // Shared buffers (lights, light view-projections) are read by lists recorded in parallel, so they are not transitioned
    // here: they stay COMMON, are promoted implicitly on read and decay when their ExecuteCommandLists
    // finishes (this engine submits one list per call). A tracked state shared
    // across lists raced and recorded barriers with matching states.
    for ( const StructuredBufferDecorator *additional_sb : additional_sbs )
    {
        additional_sb->CopySRVHeap( &temp_context );
    }

    if ( !shader_bypass )
    {
        gi.BindGraphic( &temp_context, m_light_pass_shader_raw_ );
    }

    gi.Draw( &temp_context, nullptr, 1, 0 );

    if ( postrender_predicate )
    {
        postrender_predicate( &temp_context );
    }

    for ( const auto &func : postrender_predicates | std::views::values )
    {
        func( &temp_context );
    }


    sb.TransitionCommon( &temp_context );
    const Abstracts::Resource* srv_ptrs[ g_deferred_count ];
    for ( size_t i = 0; i < g_deferred_count; ++i ) srv_ptrs[ i ] = m_deferred_render_targets_raw_[ i ];
    gi.TransitBackMultiple( &temp_context, srv_ptrs, g_deferred_count, BIND_TYPE_SRV );

    temp_context.commandList->FlagReady();
}

void Engine::DeferredRenderPassTask::RecordUsedTexture( const IGraphicContext *context,
                                                        IGraphicAPI                       &gi,
                                                        const Resources::Texture               *tex )
{
    auto tt = SingletonSpinLock::GetInstance().Lock( m_texture_record_ticket_ );
    if ( std::ranges::find( m_used_shader_textures_, tex ) == m_used_shader_textures_.end() )
    {
        gi.TransitTo( context, tex, BIND_TYPE_SRV );
        if ( const auto &empty_slot = std::ranges::find( m_used_shader_textures_, nullptr );
             empty_slot == m_used_shader_textures_.end() )
        {
            m_used_shader_textures_.push_back( tex );
        }
        else
        {
            *empty_slot = tex;
        }
    }
}

void Engine::DeferredRenderPassTask::PreRun( const RenderMap *render_map,
        const size_t render_map_count,
        const ObjectPredication &predication)
{}

Engine::RenderPassTask* Engine::DeferredRenderPassTaskFactory::New()
{
    auto* task = static_cast<DeferredRenderPassTask*>( RenderPassTaskFactory<DeferredRenderPassTask>::New() );

    task->SetDepthStencil( m_deferred_depth_.get() );
    task->SetLightShader( m_light_pass_shader_.get() );
    
    for (size_t i = 0; i < std::size(m_deferred_render_targets_); ++i)
    {
        task->SetTexture( m_deferred_render_targets_[ i ].get(), i );
    }

    return task;
}

void Engine::DeferredRenderPassTaskFactory::SetTexture( const Weak<Resources::Texture2D>& tex, const size_t slot )
{
    if (const Strong<Resources::Texture2D>& locked = tex.lock())
    {
        m_deferred_render_targets_[ slot ] = locked;
    }
}

void Engine::DeferredRenderPassTaskFactory::SetDepthStencil( const Weak<Resources::Texture2D>& tex )
{
    if (const Strong<Resources::Texture2D>& locked = tex.lock())
    {
        m_deferred_depth_ = locked;
    }
}

void Engine::DeferredRenderPassTaskFactory::SetLightShader( const Weak<Resources::Shader>& shader )
{
    if (const Strong<Resources::Shader>& locked = shader.lock())
    {
        m_light_pass_shader_ = locked;
    }
}

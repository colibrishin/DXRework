#include "ModelRendererRenderTask.h"
#include <cassert>
#include <tbb/concurrent_vector.h>
#include <tbb/parallel_for_each.h>
#include <tbb/enumerable_thread_specific.h>

#include <iterator>
#include <map>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <tbb/parallel_for.h>
#include <windows.h>

#include "DirtyCheck.h"
#include "InstanceBuild.h"
#include "ProxyCheck.h"
#include "InstanceModelSB.h"
#include "ModelRenderer.h"
#include "RenderPipeline.h"
#include "Renderer.h"
#include "SceneManager.h"

#include "Animator.h"
#include "AtlasAnimationTexture.h"
#include "ObjectBase.h"
#include "Shape.h"
#include "ShapeRenderComponent.h"
#include "Transform.h"

MODULE_IMPL( Engine::ModelRendererRenderInstanceTaskModule, ModelRendererRenderInstanceTask )

namespace Engine
{
    bool ModelRendererRenderInstanceTaskModule::InitializeImpl()
    {
        Managers::Renderer::GetInstance().RegisterRenderInstance( L"ModelRendererRenderInstanceTask",
                                                                  new ModelRendererRenderInstanceTask() );

        return true;
    }

    bool ModelRendererRenderInstanceTaskModule::ShutdownImpl()
    {
        Managers::Renderer::GetInstance().UnregisterRenderInstance( L"ModelRendererRenderInstanceTask" );

        return true;
    }

    bool ModelRendererRenderInstanceTaskModule::DynamicLoadable()
    {
        return true;
    }

    namespace
    {
        // What every object of one shape shares: resolved once per shape and thread in a Run, instead of locking the
        // material, shader, mesh and textures (shared control blocks) once per object.
        struct ResolvedMesh
        {
            size_t                      domain = 0;
            Strong<Abstracts::Resource> shader;
            Strong<Abstracts::Resource> mesh;
            Strong<Resources::Material> material;
            std::array<Strong<Abstracts::Resource>, g_max_texture_per_material> textures;
            Strong<Abstracts::Resource> atlas;
        };

        struct ResolvedShape
        {
            Strong<Resources::Shape>    shape; // keeps the address (the cache key) from being reused within the Run
            std::vector<ResolvedMesh>   meshes;
            Strong<Abstracts::Resource> animations;
        };

        // Instances of one (domain, shader, mesh) collected by one thread.
        struct Bucket
        {
            Strong<Abstracts::Resource>  shader;
            Strong<Abstracts::Resource>  mesh;
            aligned_vector<InstancePair> instances;
        };

        using BucketKey = std::tuple<size_t, const Abstracts::Resource*, const Abstracts::Resource*>;

        // Per-thread state of one Run: no shared map or lock is touched per object.
        struct ThreadState
        {
            std::unordered_map<const Resources::Shape*, ResolvedShape> shapes;
            std::map<BucketKey, Bucket>                                buckets;
            aligned_vector<Graphics::SBs::InstanceSB*>                 spare_instances;
            bool                                                       any_renderer = false;

            const ResolvedShape& Resolve( const Strong<Resources::Shape>& shape, const size_t map_size )
            {
                const auto [ it, inserted ] = shapes.try_emplace( shape.get() );
                if ( !inserted )
                {
                    return it->second;
                }

                ResolvedShape& resolved = it->second;
                resolved.shape          = shape;
                resolved.animations     = shape->GetAnimations().lock();

                for ( const auto& [ mesh, mtr ] : shape->GetMeshes() )
                {
                    if ( mesh.expired() || mtr.expired() )
                    {
                        continue;
                    }

                    Strong<Resources::Material> material = mtr.lock();
                    if ( !material )
                    {
                        continue;
                    }

                    Strong<Resources::ShaderBase> shader = material->GetShader().lock();
                    if ( !shader || shader->GetShaderDomain() >= map_size )
                    {
                        continue;
                    }

                    Strong<Resources::Mesh> locked_mesh = mesh.lock();
                    if ( !locked_mesh )
                    {
                        continue;
                    }

                    ResolvedMesh& out = resolved.meshes.emplace_back();
                    out.domain        = shader->GetShaderDomain();
                    out.shader        = std::move( shader );
                    out.mesh          = std::move( locked_mesh );

                    const auto& textures = material->GetTextures();
                    for ( size_t idx = 0; idx < textures.size(); ++idx )
                    {
                        if ( textures[ idx ].empty() )
                        {
                            continue;
                        }
                        if ( Strong<Resources::Texture> locked = textures[ idx ].lock() )
                        {
                            out.textures[ idx ] = std::move( locked );
                        }
                    }

                    if ( Strong<Resources::AtlasAnimationTexture> atlas = material->GetAtlasTexture().lock() )
                    {
                        out.atlas = std::move( atlas );
                    }

                    out.material = std::move( material );
                }

                return resolved;
            }
        };
    }

    bool VisibleInHierarchy( const Abstracts::ObjectBase& object )
    {
        if ( !object.GetActive() )
        {
            return false;
        }
        for ( Strong<Abstracts::ObjectBase> parent = object.GetParent().lock(); parent; parent = parent->GetParent().lock() )
        {
            if ( !parent->GetActive() )
            {
                return false;
            }
        }
        return true;
    }

    void BuildAllInstances( const Scene& scene, RenderMap* render_map, const size_t map_size, const BuildOptions& options )
    {
        const auto& mrs = scene.GetCachedComponentsConcurrent<Components::ModelRenderer>();

        tbb::enumerable_thread_specific<ThreadState> threads;

        tbb::parallel_for_each(
                mrs.begin(),
                mrs.end(),
                [ & ]( const Weak<Abstracts::Component>& comp )
                {
                    const Strong<Abstracts::Component> raw_component = comp.lock();
                    if ( !raw_component || !raw_component->GetActive() )
                    {
                        return;
                    }

                    // Strong locals own the objects for this scope (RAII); the raw pointers taken from them are
                    // borrowed and valid while those locals live (pools never move their objects).
                    const auto* mr = static_cast<const Components::ModelRenderer*>( raw_component.get() );
                    const Strong<Abstracts::ObjectBase> obj = raw_component->GetOwner().lock();
                    if ( !obj )
                    {
                        return;
                    }
                    const Strong<Resources::Shape>      shape = mr->GetShape().lock();
                    const Strong<Components::Transform> tr    = obj->GetComponent<Components::Transform>().lock();

                    // Nothing to render or no position information given.
                    if ( !shape || !tr )
                    {
                        return;
                    }
                    if ( options.skip_hidden && !VisibleInHierarchy( *obj ) )
                    {
                        return;
                    }

                    ThreadState& state = threads.local();
                    state.any_renderer = true;

                    const auto                         world = tr->GetWorldMatrix().Transpose();
                    const Strong<Components::Animator> anim  = obj->GetComponent<Components::Animator>().lock();
                    const ResolvedShape&               resolved = state.Resolve( shape, map_size );

                    for ( const ResolvedMesh& mesh : resolved.meshes )
                    {
                        auto [ it, inserted ] = state.buckets.try_emplace(
                                BucketKey{ mesh.domain, mesh.shader.get(), mesh.mesh.get() } );
                        Bucket& bucket = it->second;
                        if ( inserted )
                        {
                            bucket.shader = mesh.shader;
                            bucket.mesh   = mesh.mesh;
                        }

                        if ( state.spare_instances.empty() )
                        {
                            options.acquire( 64, state.spare_instances );
                        }

                        // Filled locally and moved into the bucket last, so no reference into the bucket is held while
                        // other code runs (spares not used by the end of the Run are reclaimed by Cleanup).
                        InstancePair instance_pair;
                        instance_pair.object   = obj;
                        instance_pair.instance = state.spare_instances.back();
                        state.spare_instances.pop_back();
                        if ( options.reset_instances )
                        {
                            // As the pool hands out a new buffer: zeroed, then constructed (matrices become identity).
                            std::memset( instance_pair.instance, 0, sizeof( Graphics::SBs::InstanceSB ) );
                            new ( instance_pair.instance ) Graphics::SBs::InstanceSB();
                        }

                        instance_pair.instance->SetParam( 0, world );

                        // Copy the material primitive and animator primitive if it exists.
                        mesh.material->GetPrimitive().Apply( *instance_pair.instance );

                        if ( anim )
                        {
                            anim->GetPrimitive().Apply( *instance_pair.instance );
                        }

                        for ( size_t idx = 0; idx < mesh.textures.size(); ++idx )
                        {
                            if ( mesh.textures[ idx ] )
                            {
                                instance_pair.textures[ idx ] = mesh.textures[ idx ];
                            }
                        }

                        if ( resolved.animations )
                        {
                            instance_pair.reservedTextures[ RESERVED_USER_TEX_BONES - RESERVED_USER_TEX_BEGIN ] =
                                    resolved.animations;
                        }

                        if ( mesh.atlas )
                        {
                            instance_pair.reservedTextures[ RESERVED_USER_TEX_ATLAS - RESERVED_USER_TEX_BEGIN ] =
                                    mesh.atlas;
                        }

                        bucket.instances.push_back( std::move( instance_pair ) );
                    }
                } );

        // Merge the per-thread buckets: one map access per (domain, shader, mesh), and each destination list is
        // reserved once for all threads (reserving per thread moved every element accumulated so far again for each
        // thread; an InstancePair is ~700 bytes).
        bool                                      any_renderer = false;
        std::map<BucketKey, std::vector<Bucket*>> merged;
        for ( ThreadState& state : threads )
        {
            any_renderer |= state.any_renderer;
            for ( auto& [ key, bucket ] : state.buckets )
            {
                merged[ key ].push_back( &bucket );
            }
        }

        for ( auto& [ key, buckets ] : merged )
        {
            RenderMap::accessor acc;
            if ( !render_map[ std::get<0>( key ) ].find( acc, Components::ModelRenderer::StaticTypeHash() ) )
            {
                render_map[ std::get<0>( key ) ].insert( acc, Components::ModelRenderer::StaticTypeHash() );
            }

            ShaderMap::accessor mesh_acc;
            if ( !acc->second.find( mesh_acc, buckets.front()->shader ) )
            {
                acc->second.insert( mesh_acc, buckets.front()->shader );
            }

            MeshMap::accessor shader_acc;
            if ( !mesh_acc->second.find( shader_acc, buckets.front()->mesh ) )
            {
                mesh_acc->second.insert( shader_acc, buckets.front()->mesh );
            }

            aligned_vector<InstancePair>& instances = shader_acc->second;
            if ( instances.empty() && buckets.size() == 1 )
            {
                instances = std::move( buckets.front()->instances );
                continue;
            }

            size_t total = instances.size();
            for ( const Bucket* bucket : buckets )
            {
                total += bucket->instances.size();
            }
            instances.reserve( total );

            for ( Bucket* bucket : buckets )
            {
                std::move( bucket->instances.begin(), bucket->instances.end(), std::back_inserter( instances ) );
            }
        }

        // Pre-mapping by the shader domain (as before: every domain gets the renderer key once anything renders).
        if ( any_renderer )
        {
            for ( size_t i = 0; i < map_size; ++i )
            {
                RenderMap::accessor acc;
                if ( !render_map[ i ].find( acc, Components::ModelRenderer::StaticTypeHash() ) )
                {
                    render_map[ i ].insert( acc, Components::ModelRenderer::StaticTypeHash() );
                }
            }
        }
    }


    // ---- Persistent render list (proxy mode) ----------------------------------------------------------------------
    //
    // One proxy per ModelRenderer keeps its InstancePairs in the render map across frames, each with its own
    // instance buffer. Every frame the proxy's inputs are evaluated again; when its draw lists (domain, shader,
    // mesh, material, textures) or visibility changed its pairs are moved, otherwise only the buffer values are
    // written. The map is changed only here, on the main thread, outside the parallel sections.

    struct ModelRendererRenderInstanceTask::ProxyTable
    {
        struct Entry // one InstancePair of a proxy
        {
            size_t                      domain = 0;
            Strong<Abstracts::Resource> shader;
            Strong<Abstracts::Resource> mesh;
            const void*                 material = nullptr;
            std::array<const void*, g_max_texture_per_material + 2> textures{};
            size_t                      slot = 0;
            Graphics::SBs::InstanceSB*  sb   = nullptr;

            BucketKey Key() const
            {
                return BucketKey{ domain, shader.get(), mesh.get() };
            }
        };

        struct Proxy
        {
            std::vector<Entry> entries;
        };

        // What a renderer should draw this frame (evaluated in parallel).
        struct Desired
        {
            GlobalEntityID                component = 0;
            bool                          visible   = false;
            Strong<Abstracts::ObjectBase> object;
            Matrix                        world;
            Strong<Components::Animator>  animator;
            const ResolvedShape*          resolved = nullptr;
            Proxy*                        proxy    = nullptr;
            bool                          moved    = false;
        };

        GlobalEntityID                                                   scene_id = 0;
        std::unordered_map<GlobalEntityID, Proxy>                        proxies; // by ModelRenderer component id
        std::map<BucketKey, std::vector<std::pair<GlobalEntityID, size_t>>> owners;  // per list: (component, entry)
        size_t                                                           visible_entries = 0;
        aligned_vector<Graphics::SBs::InstanceSB*>                       free_buffers;
    };

    namespace
    {
        using ProxyTable = ModelRendererRenderInstanceTask::ProxyTable;

        std::array<const void*, g_max_texture_per_material + 2> TexturesOf( const ResolvedShape& shape, const ResolvedMesh& mesh )
        {
            std::array<const void*, g_max_texture_per_material + 2> out{};
            for ( size_t i = 0; i < mesh.textures.size(); ++i )
            {
                out[ i ] = mesh.textures[ i ].get();
            }
            out[ g_max_texture_per_material ]     = shape.animations.get();
            out[ g_max_texture_per_material + 1 ] = mesh.atlas.get();
            return out;
        }

        bool SameLists( const ProxyTable::Desired& desired, const ProxyTable::Proxy& proxy )
        {
            const size_t wanted = desired.visible ? desired.resolved->meshes.size() : 0;
            if ( proxy.entries.size() != wanted )
            {
                return false;
            }
            for ( size_t i = 0; i < wanted; ++i )
            {
                const ResolvedMesh&      mesh  = desired.resolved->meshes[ i ];
                const ProxyTable::Entry& entry = proxy.entries[ i ];
                if ( entry.domain != mesh.domain || entry.shader.get() != mesh.shader.get() ||
                     entry.mesh.get() != mesh.mesh.get() || entry.material != mesh.material.get() ||
                     entry.textures != TexturesOf( *desired.resolved, mesh ) )
                {
                    return false;
                }
            }
            return true;
        }

        void ResetBuffer( Graphics::SBs::InstanceSB* sb )
        {
            std::memset( sb, 0, sizeof( Graphics::SBs::InstanceSB ) );
            new ( sb ) Graphics::SBs::InstanceSB();
        }
    }

    namespace
    {
    void InsertEntry( ProxyTable& table, RenderMap* render_map, const GlobalEntityID component, const size_t index,
                      ProxyTable::Entry& entry, InstancePair&& pair )
    {
        RenderMap::accessor acc;
        if ( !render_map[ entry.domain ].find( acc, Components::ModelRenderer::StaticTypeHash() ) )
        {
            render_map[ entry.domain ].insert( acc, Components::ModelRenderer::StaticTypeHash() );
        }
        ShaderMap::accessor shader_acc;
        if ( !acc->second.find( shader_acc, entry.shader ) )
        {
            acc->second.insert( shader_acc, entry.shader );
        }
        MeshMap::accessor mesh_acc;
        if ( !shader_acc->second.find( mesh_acc, entry.mesh ) )
        {
            shader_acc->second.insert( mesh_acc, entry.mesh );
        }
        aligned_vector<InstancePair>& list = mesh_acc->second;
        entry.slot                         = list.size();
        list.push_back( std::move( pair ) );
        auto& owners = table.owners[ entry.Key() ];
        owners.emplace_back( component, index );
        assert( owners.size() == list.size() );
        ++table.visible_entries;
    }

    void RemoveEntry( ProxyTable& table, RenderMap* render_map, const ProxyTable::Entry& entry )
    {
        const BucketKey key    = entry.Key();
        auto&           owners = table.owners.at( key );
        bool            empty = false;
        {
            RenderMap::accessor acc;
            ShaderMap::accessor shader_acc;
            MeshMap::accessor   mesh_acc;
            const bool found = render_map[ entry.domain ].find( acc, Components::ModelRenderer::StaticTypeHash() ) &&
                               acc->second.find( shader_acc, entry.shader ) && shader_acc->second.find( mesh_acc, entry.mesh );
            assert( found );
            if ( !found )
            {
                return;
            }
            aligned_vector<InstancePair>& list = mesh_acc->second;
            assert( owners.size() == list.size() && entry.slot < list.size() );
            if ( entry.slot + 1 != list.size() )
            {
                list[ entry.slot ]   = std::move( list.back() );
                owners[ entry.slot ] = owners.back();
                const auto& [ moved_component, moved_index ] = owners[ entry.slot ];
                table.proxies.at( moved_component ).entries[ moved_index ].slot = entry.slot;
            }
            list.pop_back();
            owners.pop_back();
            empty = list.empty();
        }
        --table.visible_entries;
        if ( !empty )
        {
            return;
        }
        // Drop the empty list: its keys hold the shader and mesh, which ResourceManager could not unload otherwise.
        table.owners.erase( key );
        RenderMap::accessor acc;
        if ( !render_map[ entry.domain ].find( acc, Components::ModelRenderer::StaticTypeHash() ) )
        {
            return;
        }
        bool shader_empty = false;
        {
            ShaderMap::accessor shader_acc;
            if ( acc->second.find( shader_acc, entry.shader ) )
            {
                shader_acc->second.erase( entry.mesh );
                shader_empty = shader_acc->second.empty();
            }
        }
        if ( shader_empty )
        {
            acc->second.erase( entry.shader );
        }
    }

    void RemoveProxyEntries( ProxyTable& table, RenderMap* render_map, ProxyTable::Proxy& proxy )
    {
        // Back to front: a swap-remove only moves entries of other proxies or earlier entries of this one.
        while ( !proxy.entries.empty() )
        {
            RemoveEntry( table, render_map, proxy.entries.back() );
            table.free_buffers.push_back( proxy.entries.back().sb );
            proxy.entries.pop_back();
        }
    }
    }

    void ModelRendererRenderInstanceTask::ClearProxies( RenderMap* render_map, const size_t map_size )
    {
        for ( size_t i = 0; i < map_size; ++i )
        {
            render_map[ i ].erase( Components::ModelRenderer::StaticTypeHash() );
        }
        for ( auto& [ id, proxy ] : m_proxy_->proxies )
        {
            for ( const ProxyTable::Entry& entry : proxy.entries )
            {
                m_proxy_->free_buffers.push_back( entry.sb );
            }
        }
        m_proxy_->proxies.clear();
        m_proxy_->owners.clear();
        m_proxy_->visible_entries = 0;
        m_proxy_->scene_id        = 0;
    }

    Graphics::SBs::InstanceSB* ModelRendererRenderInstanceTask::TakeBuffer()
    {
        if ( !m_proxy_->free_buffers.empty() )
        {
            Graphics::SBs::InstanceSB* sb = m_proxy_->free_buffers.back();
            m_proxy_->free_buffers.pop_back();
            return sb;
        }
        SpinLockToken token = SingletonSpinLock::GetInstance().Lock( m_instance_ticket_ );
        m_used_count_       = m_allocation_count_; // never hand out a buffer of the per-frame path
        return GetInstanceLocked();
    }

    void ModelRendererRenderInstanceTask::RunProxies( const Scene& scene, RenderMap* render_map, const size_t map_size )
    {
        if ( m_proxy_->scene_id != scene.GetID() )
        {
            ClearProxies( render_map, map_size );
            m_proxy_->scene_id = scene.GetID();
        }

        const auto mrs = scene.GetCachedComponentsConcurrent<Components::ModelRenderer>();
        std::vector<ProxyTable::Desired>             desired( mrs.size() );
        tbb::enumerable_thread_specific<ThreadState> threads;

        // 1. What every renderer should draw (parallel, read only).
        tbb::parallel_for( size_t{ 0 }, mrs.size(), [ & ]( const size_t i )
        {
            const Strong<Abstracts::Component> component = mrs[ i ].lock();
            if ( !component )
            {
                return;
            }
            ProxyTable::Desired& out = desired[ i ];
            out.component            = component->GetID();
            if ( !component->GetActive() )
            {
                return;
            }
            const auto*                         mr    = static_cast<const Components::ModelRenderer*>( component.get() );
            Strong<Abstracts::ObjectBase>       obj   = component->GetOwner().lock();
            const Strong<Resources::Shape>      shape = mr->GetShape().lock();
            if ( !obj || !shape )
            {
                return;
            }
            const Strong<Components::Transform> tr = obj->GetComponent<Components::Transform>().lock();
            if ( !tr || !VisibleInHierarchy( *obj ) )
            {
                return;
            }
            out.visible  = true;
            out.world    = tr->GetWorldMatrix().Transpose();
            out.animator = obj->GetComponent<Components::Animator>().lock();
            out.resolved = &threads.local().Resolve( shape, map_size );
            out.object   = std::move( obj );
        } );

        // 2. Proxies of renderers that are gone, then lists that changed (main thread).
        std::unordered_set<GlobalEntityID> present;
        present.reserve( desired.size() );
        for ( const ProxyTable::Desired& d : desired )
        {
            if ( d.component != 0 )
            {
                present.insert( d.component );
            }
        }
        for ( auto it = m_proxy_->proxies.begin(); it != m_proxy_->proxies.end(); )
        {
            if ( !present.contains( it->first ) )
            {
                RemoveProxyEntries( *m_proxy_, render_map, it->second );
                it = m_proxy_->proxies.erase( it );
            }
            else
            {
                ++it;
            }
        }
        for ( ProxyTable::Desired& d : desired )
        {
            if ( d.component == 0 )
            {
                continue;
            }
            ProxyTable::Proxy& proxy = m_proxy_->proxies[ d.component ];
            d.proxy                  = &proxy;
            if ( SameLists( d, proxy ) )
            {
                continue;
            }
            d.moved = true;
            RemoveProxyEntries( *m_proxy_, render_map, proxy );
            if ( !d.visible )
            {
                continue;
            }
            for ( size_t j = 0; j < d.resolved->meshes.size(); ++j )
            {
                const ResolvedMesh& mesh  = d.resolved->meshes[ j ];
                ProxyTable::Entry&  entry = proxy.entries.emplace_back();
                entry.domain              = mesh.domain;
                entry.shader              = mesh.shader;
                entry.mesh                = mesh.mesh;
                entry.material            = mesh.material.get();
                entry.textures            = TexturesOf( *d.resolved, mesh );
                entry.sb                  = TakeBuffer();

                InstancePair pair;
                pair.object   = d.object;
                pair.instance = entry.sb;
                for ( size_t idx = 0; idx < mesh.textures.size(); ++idx )
                {
                    pair.textures[ idx ] = mesh.textures[ idx ];
                }
                pair.reservedTextures[ RESERVED_USER_TEX_BONES - RESERVED_USER_TEX_BEGIN ] = d.resolved->animations;
                pair.reservedTextures[ RESERVED_USER_TEX_ATLAS - RESERVED_USER_TEX_BEGIN ] = mesh.atlas;
                InsertEntry( *m_proxy_, render_map, d.component, j, entry, std::move( pair ) );
            }
        }

        // 3. Buffer values of every visible renderer (parallel; each buffer has one writer).
        tbb::parallel_for( size_t{ 0 }, desired.size(), [ & ]( const size_t i )
        {
            const ProxyTable::Desired& d = desired[ i ];
            if ( !d.visible )
            {
                return;
            }
            for ( size_t j = 0; j < d.proxy->entries.size(); ++j )
            {
                Graphics::SBs::InstanceSB* sb = d.proxy->entries[ j ].sb;
                ResetBuffer( sb );
                sb->SetParam( 0, d.world );
                d.resolved->meshes[ j ].material->GetPrimitive().Apply( *sb );
                if ( d.animator )
                {
                    d.animator->GetPrimitive().Apply( *sb );
                }
            }
        } );

        // 4. As before: every domain has the renderer key while anything is drawn, none otherwise.
        for ( size_t i = 0; i < map_size; ++i )
        {
            if ( m_proxy_->visible_entries > 0 )
            {
                RenderMap::accessor acc;
                if ( !render_map[ i ].find( acc, Components::ModelRenderer::StaticTypeHash() ) )
                {
                    render_map[ i ].insert( acc, Components::ModelRenderer::StaticTypeHash() );
                }
            }
            else
            {
                render_map[ i ].erase( Components::ModelRenderer::StaticTypeHash() );
            }
        }
    }

    void ModelRendererRenderInstanceTask::Run( Scene const* scene, RenderMap* render_map, const size_t map_size )
    {
        DirtyCheck::Run( *scene );

        if ( m_legacy_ )
        {
            BuildOptions options;
            options.acquire = [ this ]( const size_t count, aligned_vector<Graphics::SBs::InstanceSB*>& out )
            {
                GetInstances( count, out );
            };
            BuildAllInstances( *scene, render_map, map_size, options );
        }
        else
        {
            RunProxies( *scene, render_map, map_size );
        }

        ProxyCheck::Run( *scene, render_map, map_size );
    }

    void ModelRendererRenderInstanceTask::Cleanup( RenderMap* render_map, const size_t map_size )
    {
        if ( !m_legacy_ )
        {
            // The list stays across frames; Run is not called without an active scene, so drop it here then.
            if ( !Managers::SceneManager::GetInstance().GetActiveScene().lock() )
            {
                ClearProxies( render_map, map_size );
            }
            return;
        }

        for ( size_t i = 0; i < map_size; ++i )
        {
            auto& domain_map = render_map[ i ];
            domain_map.erase( Components::ModelRenderer::StaticTypeHash() );
        }

        m_used_count_ = 0;
    }

    void ModelRendererRenderInstanceTask::Release( RenderMap* render_map, const size_t map_size )
    {
        if ( m_legacy_ )
        {
            Cleanup( render_map, map_size );
            return;
        }
        ClearProxies( render_map, map_size );
    }

    void ModelRendererRenderInstanceTask::GetInstances( const size_t count, aligned_vector<Graphics::SBs::InstanceSB*>& out )
    {
        SpinLockToken token = SingletonSpinLock::GetInstance().Lock( m_instance_ticket_ );
        out.reserve( out.size() + count );
        for ( size_t i = 0; i < count; ++i )
        {
            out.push_back( GetInstanceLocked() );
        }
    }

    // Called under m_instance_ticket_.
    Graphics::SBs::InstanceSB* ModelRendererRenderInstanceTask::GetInstanceLocked()
    {
        if ( m_allocation_count_ > m_used_count_ )
        {
            return m_instance_generated_[ m_used_count_++ ];
        }

        auto& alloc = get_instance_sb_pool_allocator();
        Graphics::SBs::InstanceSB* generated = alloc.allocate( 1 );
        try
        {
            std::memset( generated, 0, sizeof( decltype( *generated ) ) );
            alloc.construct( generated );
        }
        catch ( ... )
        {
            alloc.deallocate( generated, 1 );
            throw;
        }
        try
        {
            m_instance_generated_.push_back( generated );
        }
        catch ( ... )
        {
            alloc.destroy( generated );
            alloc.deallocate( generated, 1 );
            throw;
        }
        ++m_allocation_count_;
        ++m_used_count_;
        return generated;
    }
    namespace
    {
        // KOLIBRI_RENDER_LEGACY=1 keeps the per-frame rebuild (read in every build: it is the rollback switch).
        bool LegacyRequested()
        {
            char*  value  = nullptr;
            size_t length = 0;
            bool   legacy = false;
            if ( _dupenv_s( &value, &length, "KOLIBRI_RENDER_LEGACY" ) == 0 && value )
            {
                legacy = value[ 0 ] == '1';
                free( value );
            }
            return legacy;
        }
    }

    ModelRendererRenderInstanceTask::ModelRendererRenderInstanceTask()
        : m_instance_ticket_( SingletonSpinLock::GetInstance().Register() ),
          m_legacy_( LegacyRequested() ),
          m_proxy_( std::make_unique<ProxyTable>() )
    {
        OutputDebugStringA( m_legacy_ ? "KOLIBRI render: per-frame instance build (KOLIBRI_RENDER_LEGACY)\n"
                                      : "KOLIBRI render: persistent instance list\n" );
    }

    ModelRendererRenderInstanceTask::~ModelRendererRenderInstanceTask()
    {
        auto& alloc = get_instance_sb_pool_allocator();
        for ( auto* ptr : m_instance_generated_ )
        {
            alloc.destroy( ptr );
            alloc.deallocate( ptr );
        }
    }

} // namespace Engine

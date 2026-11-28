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
#include "ChangeCollector.h"

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
            // Every material and shader the shape refers to, also those of meshes skipped here: a change to one of
            // them can make a mesh drawable (the persistent list watches their revisions).
            std::vector<Strong<Abstracts::Resource>> watch;
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
                    Strong<Resources::Material>   material = mtr.lock();
                    Strong<Resources::ShaderBase> shader   = material ? material->GetShader().lock() : nullptr;
                    if ( material )
                    {
                        resolved.watch.push_back( material );
                    }
                    if ( shader )
                    {
                        resolved.watch.push_back( shader );
                    }
                    if ( mesh.expired() || !material )
                    {
                        continue;
                    }

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
    // instance buffer. Change events decide what is evaluated again:
    // - a scene reset, or any renderer, component or object added, removed or moved to another owner: every renderer
    //   (this also finds new and removed renderers);
    // - another component event on an object, an object's Active or Reparented event (with its descendants), or a
    //   shape/material/shader revision moving: those renderers' lists and buffers;
    // - a Transform event (with its descendants): only the world matrices.
    // The map is changed only here, on the main thread, outside the parallel sections.

    struct ModelRendererRenderInstanceTask::ProxyTable
    {
        struct Entry // one InstancePair of a proxy
        {
            size_t                      domain = 0;
            Strong<Abstracts::Resource> shader;
            Strong<Abstracts::Resource> mesh;
            Strong<Resources::Material> material;
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
            Weak<Abstracts::Component>    component;
            GlobalEntityID                object_id = 0;
            bool                          visible = false;
            Strong<Abstracts::ObjectBase> object; // while visible
            Strong<Resources::Shape>      shape;  // while visible
            std::vector<Entry>            entries;
            std::vector<Strong<Abstracts::Resource>> watched; // registered in users
        };

        // What a renderer should draw this frame (evaluated in parallel).
        struct Desired
        {
            GlobalEntityID                component = 0;
            Weak<Abstracts::Component>    weak;
            GlobalEntityID                object_id = 0;
            bool                          visible   = false;
            Strong<Abstracts::ObjectBase> object;
            Matrix                        world;
            Strong<Components::Animator>  animator;
            const ResolvedShape*          resolved = nullptr;
            Proxy*                        proxy    = nullptr;
        };

        // Proxies whose lists depend on a resource, and the revision they were built with.
        struct Users
        {
            uint64_t                                   revision = 0;
            std::unordered_map<GlobalEntityID, size_t> components; // component -> uses
        };

        GlobalEntityID                                                      scene_id = 0;
        std::unordered_map<GlobalEntityID, Proxy>                           proxies; // by ModelRenderer component id
        std::unordered_map<GlobalEntityID, std::vector<GlobalEntityID>>     by_object;
        std::map<BucketKey, std::vector<std::pair<GlobalEntityID, size_t>>> owners; // per list: (component, entry)
        std::unordered_map<const Abstracts::Resource*, Users>               users;
        size_t                                                              visible_proxies = 0;
        aligned_vector<Graphics::SBs::InstanceSB*>                          free_buffers;
        std::unique_ptr<ChangeCollector>                                    collector;
        std::vector<ChangeEvent>                                            events;
        std::unordered_set<GlobalEntityID>                                  relist; // components
        std::unordered_set<GlobalEntityID>                                  moved;  // objects
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
            // The pairs hold the object; the shape's revision is watched through proxy.shape.
            if ( ( desired.visible ? desired.object.get() : nullptr ) != proxy.object.get() ||
                 ( desired.visible ? desired.resolved->shape.get() : nullptr ) != proxy.shape.get() )
            {
                return false;
            }
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
                     entry.mesh.get() != mesh.mesh.get() || entry.material.get() != mesh.material.get() ||
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

        ProxyTable::Desired Evaluate( const Strong<Abstracts::Component>& component, ThreadState& thread, const size_t map_size )
        {
            ProxyTable::Desired out;
            out.component = component->GetID();
            out.weak      = component;
            Strong<Abstracts::ObjectBase> obj = component->GetOwner().lock();
            if ( !obj )
            {
                return out;
            }
            out.object_id = obj->GetID();
            if ( !component->GetActive() )
            {
                return out;
            }
            const auto*                    mr    = static_cast<const Components::ModelRenderer*>( component.get() );
            const Strong<Resources::Shape> shape = mr->GetShape().lock();
            if ( !shape )
            {
                return out;
            }
            const Strong<Components::Transform> tr = obj->GetComponent<Components::Transform>().lock();
            if ( !tr || !VisibleInHierarchy( *obj ) )
            {
                return out;
            }
            out.visible  = true;
            out.world    = tr->GetWorldMatrix().Transpose();
            out.animator = obj->GetComponent<Components::Animator>().lock();
            out.resolved = &thread.Resolve( shape, map_size );
            out.object   = std::move( obj );
            return out;
        }

        void AddUse( ProxyTable& table, const Abstracts::Resource* resource, const GlobalEntityID component )
        {
            ProxyTable::Users& users = table.users[ resource ];
            if ( users.components.empty() )
            {
                users.revision = resource->GetRevision();
            }
            ++users.components[ component ];
        }

        void DropUse( ProxyTable& table, const Abstracts::Resource* resource, const GlobalEntityID component )
        {
            const auto it = table.users.find( resource );
            if ( it == table.users.end() )
            {
                return;
            }
            const auto user = it->second.components.find( component );
            if ( user != it->second.components.end() && --user->second == 0 )
            {
                it->second.components.erase( user );
            }
            if ( it->second.components.empty() )
            {
                table.users.erase( it );
            }
        }

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
        }

        void RemoveEntry( ProxyTable& table, RenderMap* render_map, const GlobalEntityID component, const ProxyTable::Entry& entry )
        {
            const BucketKey key    = entry.Key();
            auto&           owners = table.owners.at( key );
            bool            empty  = false;
            {
                RenderMap::accessor acc;
                ShaderMap::accessor shader_acc;
                MeshMap::accessor   mesh_acc;
                const bool found = render_map[ entry.domain ].find( acc, Components::ModelRenderer::StaticTypeHash() ) &&
                                   acc->second.find( shader_acc, entry.shader ) &&
                                   shader_acc->second.find( mesh_acc, entry.mesh );
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

        void RemoveProxyEntries( ProxyTable& table, RenderMap* render_map, const GlobalEntityID component, ProxyTable::Proxy& proxy )
        {
            // Back to front: a swap-remove only moves entries of other proxies or earlier entries of this one.
            while ( !proxy.entries.empty() )
            {
                RemoveEntry( table, render_map, component, proxy.entries.back() );
                table.free_buffers.push_back( proxy.entries.back().sb );
                proxy.entries.pop_back();
            }
            proxy.shape.reset();
            proxy.object.reset();
        }

        void SetWatched( ProxyTable& table, const GlobalEntityID component, ProxyTable::Proxy& proxy,
                         std::vector<Strong<Abstracts::Resource>>&& watch )
        {
            if ( watch == proxy.watched )
            {
                return;
            }
            for ( const Strong<Abstracts::Resource>& resource : proxy.watched )
            {
                DropUse( table, resource.get(), component );
            }
            proxy.watched = std::move( watch );
            for ( const Strong<Abstracts::Resource>& resource : proxy.watched )
            {
                AddUse( table, resource.get(), component );
            }
        }

        void Unindex( ProxyTable& table, const GlobalEntityID object, const GlobalEntityID component )
        {
            const auto it = table.by_object.find( object );
            if ( it == table.by_object.end() )
            {
                return;
            }
            std::erase( it->second, component );
            if ( it->second.empty() )
            {
                table.by_object.erase( it );
            }
        }

        void WriteValues( const ProxyTable::Desired& d )
        {
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
        }

        // The object and its descendants.
        void CollectSubtree( const GlobalEntityID id, const Strong<Abstracts::ObjectBase>& object,
                             std::unordered_set<GlobalEntityID>& out )
        {
            if ( !out.insert( id ).second || !object )
            {
                return;
            }
            for ( const Weak<Abstracts::ObjectBase>& weak : object->GetChildren() )
            {
                if ( const Strong<Abstracts::ObjectBase> child = weak.lock() )
                {
                    CollectSubtree( child->GetID(), child, out );
                }
            }
        }
    }

    void ModelRendererRenderInstanceTask::ApplyDesired( RenderMap* render_map, void* desired_ptr )
    {
        ProxyTable::Desired& d     = *static_cast<ProxyTable::Desired*>( desired_ptr );
        ProxyTable::Proxy&   proxy = m_proxy_->proxies[ d.component ];
        d.proxy                    = &proxy;
        proxy.component            = d.weak;
        if ( proxy.object_id != d.object_id )
        {
            Unindex( *m_proxy_, proxy.object_id, d.component );
            proxy.object_id = d.object_id;
            if ( d.object_id != 0 )
            {
                m_proxy_->by_object[ d.object_id ].push_back( d.component );
            }
        }
        // What decides the lists: the shape and every material and shader it refers to (watched also while nothing of
        // it can be drawn yet).
        std::vector<Strong<Abstracts::Resource>> watch;
        if ( d.visible )
        {
            watch.reserve( d.resolved->watch.size() + 1 );
            watch.push_back( d.resolved->shape );
            watch.insert( watch.end(), d.resolved->watch.begin(), d.resolved->watch.end() );
        }
        SetWatched( *m_proxy_, d.component, proxy, std::move( watch ) );
        if ( proxy.visible != d.visible )
        {
            proxy.visible = d.visible;
            d.visible ? ++m_proxy_->visible_proxies : --m_proxy_->visible_proxies;
        }
        if ( SameLists( d, proxy ) )
        {
            return;
        }
        RemoveProxyEntries( *m_proxy_, render_map, d.component, proxy );
        if ( !d.visible )
        {
            return;
        }
        proxy.object = d.object;
        proxy.shape  = d.resolved->shape;
        for ( size_t j = 0; j < d.resolved->meshes.size(); ++j )
        {
            const ResolvedMesh& mesh  = d.resolved->meshes[ j ];
            ProxyTable::Entry&  entry = proxy.entries.emplace_back();
            entry.domain              = mesh.domain;
            entry.shader              = mesh.shader;
            entry.mesh                = mesh.mesh;
            entry.material            = mesh.material;
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
        m_proxy_->by_object.clear();
        m_proxy_->owners.clear();
        m_proxy_->users.clear();
        m_proxy_->visible_proxies = 0;
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
        m_used_count_       = m_allocation_count_; // the per-frame path's buffers are not used in this mode
        return GetInstanceLocked();
    }

    void ModelRendererRenderInstanceTask::RunProxies( const Scene& scene, RenderMap* render_map, const size_t map_size )
    {
        ProxyTable& table = *m_proxy_;
        if ( !table.collector )
        {
            table.collector = std::make_unique<ChangeCollector>();
        }
        table.collector->Drain( table.events );

        bool everything = false;
        if ( table.scene_id != scene.GetID() )
        {
            ClearProxies( render_map, map_size );
            table.scene_id = scene.GetID();
            everything     = true;
        }

        // Resources whose revision moved: their users are listed again.
        table.relist.clear();
        table.moved.clear();
        for ( auto& [ resource, users ] : table.users )
        {
            const uint64_t revision = resource->GetRevision();
            if ( revision != users.revision )
            {
                users.revision = revision;
                for ( const auto& [ component, uses ] : users.components )
                {
                    table.relist.insert( component );
                }
            }
        }

        constexpr uint16_t membership = eChange::Added | eChange::Removed | eChange::Owner;
        std::unordered_set<GlobalEntityID> relist_objects;
        for ( const ChangeEvent& event : table.events )
        {
            const auto bits = static_cast<uint16_t>( event.kind );
            if ( event.kind == eChange::SceneReset || ( bits & membership ) )
            {
                everything = true;
                break;
            }
            if ( event.component != 0 )
            {
                relist_objects.insert( event.object ); // any component of the object: its renderers again
            }
            else if ( bits & ( eChange::Active | eChange::Reparented ) )
            {
                relist_objects.insert( event.object );
                table.moved.insert( event.object );
            }
            else if ( bits & static_cast<uint16_t>( eChange::Transform ) )
            {
                table.moved.insert( event.object );
            }
        }

        tbb::enumerable_thread_specific<ThreadState> threads;
        std::vector<ProxyTable::Desired>             desired;
        // An object that changed (it may draw nothing itself but have drawn descendants).
        const auto find_object = [ & ]( const GlobalEntityID id ) -> Strong<Abstracts::ObjectBase>
        {
            if ( const auto it = table.by_object.find( id ); it != table.by_object.end() && !it->second.empty() )
            {
                if ( const ProxyTable::Proxy& proxy = table.proxies.at( it->second.front() ); proxy.object )
                {
                    return proxy.object;
                }
            }
            return const_cast<Scene&>( scene ).FindGameObject( id ).lock();
        };

        if ( everything )
        {
            // Every renderer of the scene: new ones get a proxy, gone ones lose theirs.
            const auto mrs = scene.GetCachedComponentsConcurrent<Components::ModelRenderer>();
            desired.resize( mrs.size() );
            tbb::parallel_for( size_t{ 0 }, mrs.size(), [ & ]( const size_t i )
            {
                if ( const Strong<Abstracts::Component> component = mrs[ i ].lock() )
                {
                    desired[ i ] = Evaluate( component, threads.local(), map_size );
                }
            } );
            std::unordered_set<GlobalEntityID> present;
            present.reserve( desired.size() );
            for ( const ProxyTable::Desired& d : desired )
            {
                if ( d.component != 0 )
                {
                    present.insert( d.component );
                }
            }
            for ( auto it = table.proxies.begin(); it != table.proxies.end(); )
            {
                if ( !present.contains( it->first ) )
                {
                    RemoveProxyEntries( table, render_map, it->first, it->second );
                    SetWatched( table, it->first, it->second, {} );
                    table.visible_proxies -= it->second.visible ? 1 : 0;
                    Unindex( table, it->second.object_id, it->first );
                    it = table.proxies.erase( it );
                }
                else
                {
                    ++it;
                }
            }
        }
        else
        {
            // Renderers of objects whose state changed (with their descendants for Active and Reparented), and users
            // of changed resources.
            // Subtrees first: an object put in the set on its own would end a later walk through it.
            std::unordered_set<GlobalEntityID> objects;
            for ( const GlobalEntityID id : relist_objects )
            {
                if ( table.moved.contains( id ) )
                {
                    CollectSubtree( id, find_object( id ), objects );
                }
            }
            for ( const GlobalEntityID id : relist_objects )
            {
                objects.insert( id );
            }
            for ( const GlobalEntityID id : objects )
            {
                if ( const auto it = table.by_object.find( id ); it != table.by_object.end() )
                {
                    table.relist.insert( it->second.begin(), it->second.end() );
                }
            }
            std::vector<Strong<Abstracts::Component>> components;
            components.reserve( table.relist.size() );
            for ( const GlobalEntityID id : table.relist )
            {
                if ( const auto it = table.proxies.find( id ); it != table.proxies.end() )
                {
                    if ( Strong<Abstracts::Component> component = it->second.component.lock() )
                    {
                        components.push_back( std::move( component ) );
                    }
                }
            }
            desired.resize( components.size() );
            tbb::parallel_for( size_t{ 0 }, components.size(), [ & ]( const size_t i )
            {
                desired[ i ] = Evaluate( components[ i ], threads.local(), map_size );
            } );
        }

        // Lists of the evaluated renderers (main thread), then their buffers (parallel; one writer per buffer).
        for ( ProxyTable::Desired& d : desired )
        {
            if ( d.component != 0 )
            {
                ApplyDesired( render_map, &d );
            }
        }
        tbb::parallel_for( size_t{ 0 }, desired.size(), [ & ]( const size_t i )
        {
            if ( desired[ i ].visible )
            {
                WriteValues( desired[ i ] );
            }
        } );

        // World matrices of moved objects and their descendants that were not evaluated above.
        if ( !everything && !table.moved.empty() )
        {
            std::unordered_set<GlobalEntityID> evaluated;
            for ( const ProxyTable::Desired& d : desired )
            {
                evaluated.insert( d.component );
            }
            std::unordered_set<GlobalEntityID> objects;
            for ( const GlobalEntityID id : table.moved )
            {
                if ( objects.contains( id ) )
                {
                    continue;
                }
                CollectSubtree( id, find_object( id ), objects );
            }
            std::vector<ProxyTable::Proxy*> proxies;
            for ( const GlobalEntityID id : objects )
            {
                if ( const auto it = table.by_object.find( id ); it != table.by_object.end() )
                {
                    for ( const GlobalEntityID component : it->second )
                    {
                        ProxyTable::Proxy& proxy = table.proxies.at( component );
                        if ( !proxy.entries.empty() && !evaluated.contains( component ) )
                        {
                            proxies.push_back( &proxy );
                        }
                    }
                }
            }
            tbb::parallel_for( size_t{ 0 }, proxies.size(), [ & ]( const size_t i )
            {
                const ProxyTable::Proxy& proxy = *proxies[ i ];
                if ( const Strong<Components::Transform> tr = proxy.object->GetComponent<Components::Transform>().lock() )
                {
                    const Matrix world = tr->GetWorldMatrix().Transpose();
                    for ( const ProxyTable::Entry& entry : proxy.entries )
                    {
                        entry.sb->SetParam( 0, world );
                    }
                }
            } );
        }

        // As before: every domain has the renderer key while any renderer is visible (even one that draws no mesh),
        // none otherwise.
        for ( size_t i = 0; i < map_size; ++i )
        {
            if ( table.visible_proxies > 0 )
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
            return; // the list stays across frames (Renderer calls Release when there is no scene to draw)
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

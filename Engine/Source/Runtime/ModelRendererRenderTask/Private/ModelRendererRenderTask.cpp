#include "ModelRendererRenderTask.h"
#include <cassert>
#include <tbb/concurrent_vector.h>
#include <tbb/parallel_for_each.h>
#include <tbb/enumerable_thread_specific.h>

#include <iterator>
#include <map>
#include <tuple>
#include <unordered_map>

#include "InstanceModelSB.h"
#include "ModelRenderer.h"
#include "RenderPipeline.h"
#include "Renderer.h"

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

    ModelRendererRenderInstanceTask::ModelRendererRenderInstanceTask()
        : m_instance_ticket_( SingletonSpinLock::GetInstance().Register() )
    { }

    ModelRendererRenderInstanceTask::~ModelRendererRenderInstanceTask()
    {
        auto& alloc = get_instance_sb_pool_allocator();
        for ( auto* ptr : m_instance_generated_ )
        {
            alloc.destroy( ptr );
            alloc.deallocate( ptr );
        }
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

    void ModelRendererRenderInstanceTask::Run( Scene const* scene, RenderMap* render_map, const size_t map_size )
    {
        const auto& mrs = scene->GetCachedComponentsConcurrent<Components::ModelRenderer>();

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
                            GetInstances( 64, state.spare_instances );
                        }

                        // Filled locally and moved into the bucket last, so no reference into the bucket is held while
                        // other code runs (spares not used by the end of the Run are reclaimed by Cleanup).
                        InstancePair instance_pair;
                        instance_pair.object   = obj;
                        instance_pair.instance = state.spare_instances.back();
                        state.spare_instances.pop_back();

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

    void ModelRendererRenderInstanceTask::Cleanup( RenderMap* render_map, const size_t map_size )
    {
        for ( size_t i = 0; i < map_size; ++i )
        {
            auto& domain_map = render_map[ i ];
            domain_map.erase( Components::ModelRenderer::StaticTypeHash() );
        }

        m_used_count_ = 0;
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
} // namespace Engine

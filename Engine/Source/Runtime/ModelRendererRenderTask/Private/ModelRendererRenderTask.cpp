#include "ModelRendererRenderTask.h"
#include <cassert>
#include <tbb/concurrent_vector.h>
#include <tbb/parallel_for_each.h>

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

    void ModelRendererRenderInstanceTask::Run( Scene const* scene, RenderMap* render_map, const size_t map_size )
    {
        const auto& mrs = scene->GetCachedComponentsConcurrent<Components::ModelRenderer>();

        tbb::parallel_for_each(
                mrs.begin(),
                mrs.end(),
                [ & ]( const Weak<Abstracts::Component>& comp )
                {
                    if ( const Strong<Abstracts::Component>& raw_component = comp.lock() )
                    {
                        // get model renderer, continue if it is disabled
                        if ( !raw_component->GetActive() )
                        {
                            return;
                        }

                        // Strong locals own the objects for this scope (RAII); the raw pointers taken from them are
                        // borrowed and valid while those locals live (pools never move their objects).
                        const Strong<Components::ModelRenderer>& mr =
                                raw_component->GetSharedPtr<Components::ModelRenderer>();
                        const Strong<Abstracts::ObjectBase>& obj   = raw_component->GetOwner().lock();
                        const Strong<Resources::Shape>       shape = mr->GetShape().lock();
                        const Strong<Components::Transform>  tr    = obj->GetComponent<Components::Transform>().lock();

                        // Nothing to render or no position information given.
                        if ( !shape || !tr )
                        {
                            return;
                        }

                        Resources::Shape* const    raw_shape = shape.get();
                        const auto                 world     = tr->GetWorldMatrix().Transpose();
                        const Strong<Components::Animator> anim =
                                obj->GetComponent<Components::Animator>().lock();
                        const Strong<Resources::AnimationTexture> anims = raw_shape->GetAnimations().lock();

                        // Pre-mapping by the shader domain.
                        for ( size_t i = 0; i < map_size; ++i )
                        {
                            auto& domain_map = render_map[ i ];

                            RenderMap::accessor acc;
                            if ( !domain_map.find( acc, Components::ModelRenderer::StaticTypeHash() ) )
                            {
                                domain_map.insert( acc, Components::ModelRenderer::StaticTypeHash() );
                            }
                        }

                        // One pass over the meshes; each goes to the domain of its shader.
                        for ( const auto& [ mesh, mtr ] : raw_shape->GetMeshes() )
                        {
                            if ( mesh.expired() || mtr.expired() )
                            {
                                continue;
                            }

                            const Strong<Resources::Material>&   locked_mtr    = mtr.lock();
                            if ( !locked_mtr )
                            {
                                continue;
                            }
                            Resources::Material* const           material      = locked_mtr.get();
                            const Strong<Resources::ShaderBase>& locked_shader = material->GetShader().lock();
                            if ( !locked_shader )
                            {
                                continue;
                            }

                            const size_t domain = locked_shader->GetShaderDomain();
                            if ( domain >= map_size )
                            {
                                continue;
                            }

                            RenderMap::accessor acc;
                            if ( !render_map[ domain ].find( acc, Components::ModelRenderer::StaticTypeHash() ) )
                            {
                                render_map[ domain ].insert( acc, Components::ModelRenderer::StaticTypeHash() );
                            }

                            ShaderMap::accessor mesh_acc;
                            if ( !acc->second.find( mesh_acc, locked_shader ) )
                            {
                                acc->second.insert( mesh_acc, locked_shader );
                            }

                            if ( const Strong<Resources::Mesh>& locked_mesh = mesh.lock() )
                            {
                                decltype( mesh_acc->second )::accessor shader_acc;
                                if ( !mesh_acc->second.find( shader_acc, locked_mesh ) )
                                {
                                    mesh_acc->second.insert( shader_acc, locked_mesh );
                                }

                                InstancePair instance_pair;
                                instance_pair.object = obj;

                                instance_pair.instance = GetInstance();
                                instance_pair.instance->SetParam( 0, world );

                                // Copy the material primitive and animator primitive if it exists.
                                material->GetPrimitive().Apply( *instance_pair.instance );

                                if ( anim )
                                {
                                    anim->GetPrimitive().Apply( *instance_pair.instance );
                                }

                                const auto& textures = material->GetTextures();
                                for ( auto it = textures.begin(); it != textures.end(); ++it )
                                {
                                    const size_t idx = std::distance( textures.begin(), it );
                                    if ( const Strong<Resources::Texture>& locked = it->lock() )
                                    {
                                        instance_pair.textures[ idx ] = locked;
                                    }
                                }

                                if ( anims )
                                {
                                    instance_pair.reservedTextures[ RESERVED_USER_TEX_BONES - RESERVED_USER_TEX_BEGIN ] =
                                            anims;
                                }

                                if ( const Strong<Resources::AtlasAnimationTexture>& atlas =
                                             material->GetAtlasTexture().lock() )
                                {
                                    instance_pair.reservedTextures[ RESERVED_USER_TEX_ATLAS - RESERVED_USER_TEX_BEGIN ] =
                                            atlas;
                                }

                                shader_acc->second.emplace_back( instance_pair );
                            }
                        }
                    }
                } );
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

    Graphics::SBs::InstanceSB* ModelRendererRenderInstanceTask::GetInstance()
    {
        SpinLockToken token = SingletonSpinLock::GetInstance().Lock( m_instance_ticket_ );

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

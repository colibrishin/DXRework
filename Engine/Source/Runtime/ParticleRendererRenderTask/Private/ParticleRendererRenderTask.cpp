#include "ParticleRendererRenderTask.h"
#include <tbb/parallel_for_each.h>

#include "ParticleRenderer.h"
#include "Renderer.h"

#include "ShapeRenderComponent.h"
#include "ObjectBase.h"
#include "Material.h"
#include "Transform.h"
#include "Shape.h"
#include "Animator.h"

namespace Engine
{
    ParticleRendererRenderInstanceTask::~ParticleRendererRenderInstanceTask()
    {
        auto& alloc = get_instance_sb_pool_allocator();
        for ( auto* ptr : m_instance_generated_ )
        {
            alloc.destroy( ptr );
            alloc.deallocate( ptr );
        }
    }

    ParticleRendererRenderInstanceTask::ParticleRendererRenderInstanceTask() :
        m_instance_ticket_(SingletonSpinLock::GetInstance().Register())
    {}

    void ParticleRendererRenderInstanceTask::Run(
        Scene const* scene,
        RenderMap*   render_map,
        const size_t map_size
    )
    {
        const auto &prs = scene->GetCachedComponentsConcurrent<Components::ParticleRenderer>();

        tbb::parallel_for_each(
                prs.begin(),
                prs.end(),
                [ & ]( const Weak<Abstracts::Component> &comp )
                {
                    if ( const Strong<Abstracts::Component> &raw_component = comp.lock() )
                    {
                        if ( !raw_component->GetActive() )
                        {
                            return;
                        }

                        const Strong<Components::ParticleRenderer> &pr =
                                raw_component->GetSharedPtr<Components::ParticleRenderer>();
                        const Strong<Abstracts::ObjectBase> &obj   = raw_component->GetOwner().lock();
                        const Strong<Resources::Shape>      &shape = pr->GetShape().lock();
                        const Strong<Components::Transform> &tr    = obj->GetComponent<Components::Transform>().lock();

                        if ( !shape || !tr )
                        {
                            return;
                        }

                        // One render map key per domain, as before (the passes look it up).
                        for ( size_t i = 0; i < map_size; ++i )
                        {
                            RenderMap::accessor acc;
                            if ( !render_map[ i ].find( acc, Components::ParticleRenderer::StaticTypeHash() ) )
                            {
                                render_map[ i ].insert( acc, Components::ParticleRenderer::StaticTypeHash() );
                            }
                        }

                        // Per emitter, not per particle: the owner transform, animator and textures.
                        const bool                                follow_owner = pr->IsFollowOwner();
                        const Matrix                              owner_world  = tr->GetWorldMatrix().Transpose();
                        const Strong<Components::Animator>        anim         = obj->GetComponent<Components::Animator>().lock();
                        const Strong<Resources::AnimationTexture> anims        = shape->GetAnimations().lock();

                        auto &particles = reinterpret_cast<aligned_vector<Graphics::SBs::InstanceSB> &>( pr->GetInstances() );
                        size_t active = 0;
                        for ( auto &particle : particles )
                        {
                            // Retired particles stay in the buffer (the simulation skips them).
                            active += reinterpret_cast<Graphics::SBs::InstanceParticleSB &>( particle ).GetActive() ? 1 : 0;
                        }
                        if ( active == 0 )
                        {
                            return;
                        }

                        for ( const auto &[ mesh, mtr ] : shape->GetMeshes() )
                        {
                            const Strong<Resources::Mesh>     locked_mesh = mesh.lock();
                            const Strong<Resources::Material> locked_mtr  = mtr.lock();
                            if ( !locked_mesh || !locked_mtr )
                            {
                                continue;
                            }

                            const Strong<Resources::ShaderBase> locked_shader = locked_mtr->GetShader().lock();
                            const auto domain = static_cast<size_t>( locked_shader->GetShaderDomain() );
                            if ( domain >= map_size )
                            {
                                continue;
                            }

                            InstancePair tmpl;
                            tmpl.object = obj;
                            for ( auto it = locked_mtr->GetTextures().begin(); it != locked_mtr->GetTextures().end(); ++it )
                            {
                                if ( const Strong<Resources::Texture> &locked = it->lock() )
                                {
                                    tmpl.textures[ std::distance( locked_mtr->GetTextures().begin(), it ) ] = locked;
                                }
                            }
                            if ( anims )
                            {
                                tmpl.reservedTextures[ RESERVED_USER_TEX_BONES - RESERVED_USER_TEX_BEGIN ] = anims;
                            }
                            if ( const Strong<Resources::AtlasAnimationTexture> &atlas = locked_mtr->GetAtlasTexture().lock() )
                            {
                                tmpl.reservedTextures[ RESERVED_USER_TEX_ATLAS - RESERVED_USER_TEX_BEGIN ] = atlas;
                            }

                            // Build outside the render map lock, so emitters run in parallel.
                            aligned_vector<InstancePair> pairs( active, tmpl );
                            GetInstances( active, pairs );
                            size_t k = 0;
                            for ( auto &particle : particles )
                            {
                                if ( !reinterpret_cast<Graphics::SBs::InstanceParticleSB &>( particle ).GetActive() )
                                {
                                    continue;
                                }

                                Graphics::SBs::InstanceSB &instance = *pairs[ k++ ].instance;
                                instance = particle;
                                if ( follow_owner )
                                {
                                    instance.SetParam( 0, owner_world * particle.GetParam<Matrix>( 0 ) );
                                }

                                locked_mtr->GetPrimitive().Apply( instance );
                                if ( anim )
                                {
                                    anim->GetPrimitive().Apply( instance );
                                }
                            }

                            RenderMap::accessor acc;
                            render_map[ domain ].find( acc, Components::ParticleRenderer::StaticTypeHash() );

                            ShaderMap::accessor shader_acc;
                            if ( !acc->second.find( shader_acc, locked_shader ) )
                            {
                                acc->second.insert( shader_acc, locked_shader );
                            }

                            decltype( shader_acc->second )::accessor mesh_acc;
                            if ( !shader_acc->second.find( mesh_acc, locked_mesh ) )
                            {
                                shader_acc->second.insert( mesh_acc, locked_mesh );
                            }

                            auto &list = mesh_acc->second;
                            list.insert( list.end(), std::make_move_iterator( pairs.begin() ), std::make_move_iterator( pairs.end() ) );
                        }
                    }
        });
    }

    void ParticleRendererRenderInstanceTask::Cleanup(RenderMap* render_map, const size_t map_size) 
    {
        for (size_t i = 0; i < map_size; ++i)
        {
            auto& domain_map = render_map[i];
            domain_map.erase(Components::ParticleRenderer::StaticTypeHash());
        }

        m_used_count_ = 0;
    }

    void ParticleRendererRenderInstanceTask::GetInstances( const size_t count, aligned_vector<InstancePair>& out )
    {
        // One lock for all particles of a mesh.
        SpinLockToken token = SingletonSpinLock::GetInstance().Lock( m_instance_ticket_ );

        auto& alloc = get_instance_sb_pool_allocator();
        for ( size_t i = 0; i < count; ++i )
        {
            if ( m_allocation_count_ == m_used_count_ )
            {
                Graphics::SBs::InstanceSB* generated = alloc.allocate( 1 );
                try
                {
                    std::memset(generated, 0, sizeof(decltype(*generated)));
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
            }
            out[ i ].instance = m_instance_generated_[ m_used_count_++ ];
        }
    }
}

#include "../Public/ParticleRendererRenderTask.h"
#include "ParticleRendererRenderTask.generated.h"
#include <tbb/parallel_for_each.h>

#include "ParticleRenderer.h"
#include "Renderer.h"

#include "Source/Runtime/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"
#include "Source/Runtime/Resources/Material/Public/Material.h"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Shape.h"

#include "Source/Runtime/Components/Animator/Public/Animator.h"

namespace Engine
{
    ParticleRendererRenderInstanceTask::~ParticleRendererRenderInstanceTask()
    {
        for (auto* ptr : m_instance_generated_)
        {
            m_instance_allocator_.destroy(ptr);
            m_instance_allocator_.deallocate(ptr);
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
        const auto& prs = scene->GetCachedComponentsConcurrent<Components::ParticleRenderer>();

        tbb::parallel_for_each(prs.begin(), prs.end(), [&](const Weak<Abstracts::Component>& comp)
        {
            if (const Strong<Abstracts::Component>& raw_component = comp.lock())
            {
                if (!raw_component->GetActive())
                {
                    return;
                }

                const Strong<Components::ParticleRenderer>& pr = raw_component->GetSharedPtr<Components::ParticleRenderer>();
                const Strong<Abstracts::ObjectBase>& obj = raw_component->GetOwner().lock();
                const Strong<Resources::Shape>& shape = pr->GetShape().lock();
                const Strong<Components::Transform>& tr  = obj->GetComponent<Components::Transform>().lock();

                if (!shape || !tr)
                {
                    return;
                }
                
                // Pre-mapping by the shader domain.
                for (auto i = 0; i < map_size; ++i)
                {
                    const auto domain = static_cast<eShaderDomain>(i);
                    auto& domain_map = render_map[domain];

                    RenderMap::accessor acc;
                    if (!domain_map.find(acc, Components::ParticleRenderer::StaticTypeHash()))
                    {
                        domain_map.insert(acc, Components::ParticleRenderer::StaticTypeHash());
                    }

                    for (const auto& [mesh, mtr] : shape->GetMeshes())
                    {
                        if (mesh.expired() || mtr.expired())
                        {
                            continue;
                        }

                        const Strong<Resources::Material>& locked_mtr = mtr.lock();
                        const Strong<Resources::Mesh>& locked_mesh = mesh.lock();

                        MeshMap::accessor mesh_acc;
                        if (!acc->second.find(mesh_acc, locked_mesh))
                        {
                            acc->second.insert(mesh_acc, locked_mesh);
                        }

                        if (const Strong<Resources::Shader>& locked_shader = locked_mtr->GetShader().lock())
                        {
                            if (locked_shader->GetDomain() != domain)
                            {
                                continue;
                            }

                            decltype(mesh_acc->second)::accessor shader_acc;
                            if (!mesh_acc->second.find(shader_acc, locked_shader))
                            {
                                mesh_acc->second.insert(shader_acc, locked_shader);
                            }

                            auto& particles = reinterpret_cast<aligned_vector<Graphics::SBs::InstanceSB>&>(pr->GetInstances());
                            
                            for (auto& particle : particles)
                            {
                                InstancePair instance_pair;
                                instance_pair.object = obj;
                                instance_pair.instance = GetInstance();

                                *instance_pair.instance = particle;
                                if (pr->IsFollowOwner())
                                {
                                    auto mat = particle.GetParam<Matrix>(0);
                                    mat = tr->GetWorldMatrix().Transpose() * mat;
                                    instance_pair.instance->SetParam(0, mat);
                                }

                                locked_mtr->GetPrimitive().Apply(*instance_pair.instance);
                                
                                if (const Strong<Components::Animator>& anim = obj->GetComponent<Components::Animator>().lock())
                                {
                                    anim->GetPrimitive().Apply(*instance_pair.instance);
                                }

                                for (auto it = locked_mtr->GetTextures().begin(); it != locked_mtr->GetTextures().end(); ++it)
                                {
                                    const size_t idx = std::distance(locked_mtr->GetTextures().begin(), it);
                                    if (const Strong<Resources::Texture>& locked = it->lock())
                                    {
                                        instance_pair.textures[idx] = locked;
                                    }
                                }

                                if (const Strong<Resources::AnimationTexture>& anims = shape->GetAnimations().lock())
                                {
                                    instance_pair.reservedTextures[RESERVED_USER_TEX_BONES - RESERVED_USER_TEX_BEGIN] = anims;
                                }
                            
                                if (const Strong<Resources::AtlasAnimationTexture>& atlas = locked_mtr->GetAtlasTexture().lock())
                                {
                                    instance_pair.reservedTextures[RESERVED_USER_TEX_ATLAS - RESERVED_USER_TEX_BEGIN] = atlas;
                                }

                                shader_acc->second.push_back(instance_pair);
                            }
                        }
                    }
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

    Graphics::SBs::InstanceSB* ParticleRendererRenderInstanceTask::GetInstance()
    {
        SpinLockToken token = SingletonSpinLock::GetInstance().Lock(m_instance_ticket_);

        if (m_allocation_count_ > m_used_count_)
        {
            return m_instance_generated_[m_used_count_++];
        }

        Graphics::SBs::InstanceSB* generated = m_instance_allocator_.allocate();

        std::memset(generated, 0, sizeof(decltype(*generated)));
        m_instance_allocator_.construct(generated);
        m_instance_generated_.push_back(generated);

        ++m_allocation_count_;
        ++m_used_count_;
        return generated;
    }
}

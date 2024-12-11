#include "../Public/ParticleRendererRenderTask.h"
#include "Source/Runtime/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Components/Animator/Public/Animator.h"
#include "Source/Runtime/Resources/BaseAnimation/Public/BaseAnimation.h"
#include "Source/Runtime/Resources/BoneAnimation/Public/BoneAnimation.h"
#include "Source/Runtime/Resources/AtlasAnimation/Public/AtlasAnimation.h"
#include "Source/Runtime/ParticleRendererExtension/Public/ParticleRendererExtension.h"

namespace Engine 
{
    void ParticleRendererRenderInstanceTask::Run(
            const Scene const* scene, 
            const RenderMapValueType* render_map, 
            const std::size_t map_size, 
            std::atomic<uint64_t>& instance_count)
    {
        const auto& rawcomponents = scene->GetCachedComponents<Components::RenderComponent>();

        tbb::parallel_for_each(rawcomponents.begin(), rawcomponents.end(), [](const Weak<Components::RenderComponent>& comp)
        {
            if (const Strong<Components::RenderComponent>& pr = comp.lock())
            {
                if (!pr->GetActive())
                {
                    return;
                }

                const Strong<Abstracts::ObjectBase>& obj = pr->GetOwner().lock();
                const Strong<Resources::Material>& mtr = pr->GetMaterial().lock();
                const Strong<Components::Transform>& tr  = obj->GetComponent<Components::Transform>().lock();

                // Pre-mapping by the material.
                for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
                {
                    const auto domain = static_cast<eShaderDomain>(i);

                    if (mtr->IsRenderDomain(domain))
                    {
                        auto particles = ParticleRendererExtension::GetInstances();

                        if (particles.empty())
                        {
                            continue;
                        }

                        auto& domain_map = out_map[domain];

                        RenderMap::accessor acc;

                        if (!domain_map.find(acc, RENDER_COM_T_PARTICLE))
                        {
                            domain_map.insert(acc, RENDER_COM_T_PARTICLE);
                        }

                        if (pr->IsFollowOwner())
                        {
                            for (auto& particle : particles)
                            {
                                auto mat = particle.GetParam<Matrix>(0);
                                mat      = tr->GetWorldMatrix().Transpose() * mat;
                                particle.SetParam(0, mat);
                            }
                        }

                        acc->second.push_back(std::make_tuple(obj, mtr, particles));
                        instance_count.fetch_add(particles.size());
                    }
                }
            }
        })
    }

    void ParticleRendererRenderTask::Cleanup(const RenderMapValueType* render_map) 
    {
        render_map->clear();
    }
}
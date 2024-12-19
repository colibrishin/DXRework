#include "../Public/ParticleRendererRenderTask.h"
#include <tbb/parallel_for_each.h>
#include "Source/Runtime/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/Resources/Material/Public/Material.h"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Resources/BoneAnimation/Public/BoneAnimation.h"
#include "Source/Runtime/ParticleRendererExtension/Public/ParticleRendererExtension.h"

namespace Engine 
{
    void ParticleRendererRenderInstanceTask::Run(
            Scene const* scene, 
            const RenderMapValueType* render_map, 
            std::atomic<uint64_t>& instance_count)
    {
        const auto& rawcomponents = scene->GetCachedComponents<Components::RenderComponent>();

        tbb::parallel_for_each(rawcomponents.begin(), rawcomponents.end(), [&instance_count, render_map](const Weak<Components::RenderComponent>& comp)
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
                        const Strong<Components::ParticleRenderer>& locked = pr->GetSharedPtr<Components::ParticleRenderer>();
                        auto& particles = reinterpret_cast<aligned_vector<Graphics::SBs::InstanceSB>&>(ParticleRendererExtension::GetInstances(locked));

                        if (particles.empty())
                        {
                            continue;
                        }

                        if (locked->IsFollowOwner())
                        {
                            for (auto& particle : particles)
                            {
                                auto mat = particle.GetParam<Matrix>(0);
                                mat      = tr->GetWorldMatrix().Transpose() * mat;
                                particle.SetParam(0, mat);
                            }
                        }

                        render_map->push_back(std::make_tuple(obj, mtr, particles));
                        instance_count.fetch_add(particles.size());
                    }
                }
            }
        });
    }

    void ParticleRendererRenderInstanceTask::Cleanup(RenderMapValueType* render_map) 
    {
        render_map->clear();
    }
}
#include "../Public/ParticleRendererRenderTask.h"
#include <tbb/parallel_for_each.h>

#include "Renderer.h"

#include "Source/Runtime/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"
#include "Source/Runtime/Resources/Material/Public/Material.h"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/ParticleRendererExtension/Public/ParticleRendererExtension.h"

namespace Engine 
{
	void ParticleRendererRenderInstanceTaskModule::Initialize()
	{
		Managers::Renderer::GetInstance().RegisterRenderInstance(L"ParticleRendererRenderInstanceTask", new ParticleRendererRenderInstanceTask());
	}

	void ParticleRendererRenderInstanceTaskModule::Shutdown()
	{
        Managers::Renderer::GetInstance().UnregisterRenderInstance(L"ParticleRendererRenderInstanceTask");
	}

	bool ParticleRendererRenderInstanceTaskModule::DynamicLoadable()
	{
		return true;
	}

    void ParticleRendererRenderInstanceTask::Run(
        Scene const* scene,
        RenderMap*   render_map,
        const size_t map_size, std::atomic<uint64_t>& instance_count
    )
    {
        const auto& prs = scene->GetCachedComponents<Components::ParticleRenderer>();

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
                const Strong<Resources::Material>& mtr = pr->GetMaterial().lock();
                const Strong<Components::Transform>& tr  = obj->GetComponent<Components::Transform>().lock();

                // Pre-mapping by the material.
                for (auto i = 0; i < map_size; ++i)
                {
                    const auto domain = static_cast<eShaderDomain>(i);

                    if (mtr->IsRenderDomain(domain))
                    {
                        auto& particles = reinterpret_cast<aligned_vector<Graphics::SBs::InstanceSB>&>(ParticleRendererExtension::GetInstances(pr));

                        if (particles.empty())
                        {
                            continue;
                        }

                        auto& domain_map = render_map[domain];

                        RenderMap::accessor acc;

                        if (!domain_map.find(acc, Components::ParticleRenderer::StaticTypeHash()))
                        {
                            domain_map.insert(acc, Components::ParticleRenderer::StaticTypeHash());
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
        });
    }

    void ParticleRendererRenderInstanceTask::Cleanup(RenderMap* render_map, const size_t map_size) 
    {
        for (size_t i = 0; i < map_size; ++i)
        {
            auto& domain_map = render_map[i];

            if (RenderMap::accessor acc;
                domain_map.find(acc, Components::ParticleRenderer::StaticTypeHash()))
            {
                domain_map.erase(Components::ParticleRenderer::StaticTypeHash());
            }
        }
    }
}
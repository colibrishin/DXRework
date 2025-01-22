#include "../Public/ParticleRendererExtension.h"

namespace Engine 
{
    Graphics::ParamBase& ParticleRendererExtension::GetParam(const Strong<Components::ParticleRenderer>& pr)
    {
        return pr->m_params_;
    }

    InstanceParticles& ParticleRendererExtension::GetInstances(const Strong<Components::ParticleRenderer>& pr)
    {
        return pr->m_instances_;
    }
}

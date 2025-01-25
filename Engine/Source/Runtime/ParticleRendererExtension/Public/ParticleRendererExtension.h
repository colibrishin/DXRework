#pragma once
#include "Source/Runtime/Components/ParticleRenderer/Public/ParticleRenderer.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine 
{
    struct ENGINE_PARTICLERENDEREREXTENSION_API ParticleRendererExtension 
    {
        static Graphics::ParamBase& GetParam(const Strong<Components::ParticleRenderer>& pr);
        static InstanceParticles&   GetInstances(const Strong<Components::ParticleRenderer>& pr);
    };
}
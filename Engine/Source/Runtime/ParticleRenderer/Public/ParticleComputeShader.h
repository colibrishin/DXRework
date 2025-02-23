#pragma once

#include "ComputeShader.h"
#include "InstanceParticleSB.h"

#include "ParticleComputeShader.generated.h"

namespace Engine::Resources
{
    ECLASS(resource, abstract, serialize)
    class ENGINE_PARTICLERENDERER_API ParticleComputeShader : public ComputeShader
    {
        GENERATE_BODY
    public:
        using ComputeShader::ComputeShader;

#if WITH_EDITOR
        virtual void OnUIUpdateParam(
            UIContext* const parent,
            const float dt,
            Graphics::ParamBase& local_param,
            InstanceParticles& instances) = 0;
#endif
    };
}

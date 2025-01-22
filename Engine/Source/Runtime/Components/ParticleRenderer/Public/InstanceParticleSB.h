#pragma once
#include "StructuredBuffer/Public/StructuredBuffer.h"

#include "InstanceParticleSB.generated.h"

namespace Engine
{
    struct ParticleRendererExtension;

    namespace Graphics::SBs
    {
        struct ENGINE_PARTICLERENDERER_API InstanceParticleSB : public InstanceSB
        {
        public:
            InstanceParticleSB();
            void SetLife(const float life);
            void SetActive(const bool active);
            void SetVelocity(const Vector3& velocity);
            void SetWorld(const Matrix& world);
            Matrix& GetWorld();
            bool& GetActive();
        };
    }

    using InstanceParticles = aligned_vector<Graphics::SBs::InstanceParticleSB>;
}

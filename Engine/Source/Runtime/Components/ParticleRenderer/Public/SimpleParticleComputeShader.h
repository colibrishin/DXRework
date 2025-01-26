#pragma once
#include <random>

#include "ComputeShader.h"
#include "InstanceParticleSB.h"

#include "SimpleParticleComputeShader.generated.h"

namespace Engine::Resources
{
    ECLASS( resource, internal, serialize )
    class SimpleParticleComputeShader final : public ComputeShader
    {
        GENERATE_BODY
        SimpleParticleComputeShader()
            : ComputeShader( "cs_particle.hlsl", { 32, 32, 1 } ) { }

    protected:
        void preDispatch(const GraphicInterfaceContextPrimitive* context, Graphics::SBs::LocalParamSB& param) override;
        void postDispatch(const GraphicInterfaceContextPrimitive* context, Graphics::SBs::LocalParamSB& param) override;
        void loadDerived() override;
        void unloadDerived() override;
        void SetScaling(bool scaling, Graphics::ParamBase& config);
        void SetScalingParam(float min, float max, Graphics::ParamBase& config);
        void LinearSpread(
            const Vector3& local_min, const Vector3& local_max, InstanceParticles& particles,
            const Graphics::ParamBase& config);

    private:
        // int
        constexpr static size_t scaling_active_slot = 1;
        constexpr static size_t random_value_slot   = 2;

        // float
        constexpr static size_t dt_slot          = 2;
        constexpr static size_t scaling_min_slot = 3;
        constexpr static size_t scaling_max_slot = 4;

        static std::mt19937_64 getRandomEngine();

        constexpr static size_t random_texture_count = 3;
        constexpr static size_t random_texture_size  = 500 * 500;

        std::array<Strong<Texture2D>, random_texture_count> m_noises_;
    };
}

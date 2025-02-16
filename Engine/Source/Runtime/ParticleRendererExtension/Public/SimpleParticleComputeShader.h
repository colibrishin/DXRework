#pragma once
#include <random>

#include "InstanceParticleSB.h"
#include "ParticleComputeShader.h"

#include "SimpleParticleComputeShader.generated.h"

namespace Engine::Resources
{
    ECLASS( resource, internal, serialize )
    class ENGINE_PARTICLERENDEREREXTENSION_API SimpleParticleComputeShader final : public ParticleComputeShader
    {
        GENERATE_BODY
        SimpleParticleComputeShader()
            : ParticleComputeShader( "cs_particle.hlsl" ) 
        {
            SetThread( { 32, 32, 1 } );
        }

#if WITH_EDITOR
        void OnUIUpdate(UIContext* const parent, const float dt) override;
        void OnUIUpdateParam(UIContext* const parent, const float dt, Graphics::ParamBase& local_param, InstanceParticles& instances) override;
#endif

    protected:
        void preDispatch(const GraphicInterfaceContextPrimitive* context, Graphics::SBs::LocalParamSB& param, const float dt) override;
        void postDispatch(const GraphicInterfaceContextPrimitive* context, Graphics::SBs::LocalParamSB& param, const float dt) override;
        void loadDerived() override;
        void unloadDerived() override;
        static void SetScaling(bool scaling, Graphics::ParamBase& config);
        static void SetScalingParam(float min, float max, Graphics::ParamBase& config);
        static void LinearSpread(
            const Vector3& local_min, const Vector3& local_max, InstanceParticles& particles,
            const Graphics::ParamBase& config);

    private:
        // int
        constexpr static size_t param_scaling_active_slot = 1;
        constexpr static size_t param_random_value_slot   = 2;

        // float
        constexpr static size_t param_dt_slot          = 2;
        constexpr static size_t param_scaling_min_slot = 3;
        constexpr static size_t param_scaling_max_slot = 4;

        static std::mt19937_64 getRandomEngine();

        constexpr static size_t random_texture_count = 3;
        constexpr static size_t random_texture_size  = 500 * 500;

        std::array<Strong<Texture2D>, random_texture_count> m_noises_;
    };
}

#pragma once
#include "egMacro.h"
#include "egRenderComponent.h"
#include "egStructuredBuffer.hpp"

namespace Engine::Components
{
  class ParticleRenderer : public Base::RenderComponent
  {
  public:
    RENDER_COM_T(RENDER_COM_T_PARTICLE)
    constexpr static size_t particle_count_slot = 1;
    constexpr static size_t dt_slot     = 2;

    ParticleRenderer(const WeakObject& owner);

    void Initialize() override;
    void Update(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;

    const std::vector<Graphics::SBs::InstanceSB>& GetParticles() const;

    void SetCount(const size_t count);
    void SetComputeShader(const WeakComputeShader& cs);
    void Spread(const Vector3& local_min, const Vector3& local_max);

  private:
    SERIALIZER_ACCESS
    ParticleRenderer();

    Graphics::StructuredBuffer<Graphics::SBs::InstanceParticleSB> m_sb_buffer_;
    std::vector<Graphics::SBs::InstanceSB>                        m_sbs_;
    std::string                                                   m_cs_name_;
    StrongComputeShader                                           m_cs_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::ParticleRenderer)

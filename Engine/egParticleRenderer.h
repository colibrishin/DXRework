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
    constexpr static size_t dt_slot             = 2;
    constexpr static size_t duration_slot       = 3;

    ParticleRenderer(const WeakObject& owner);

    void Initialize() override;
    void Update(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;

    const std::vector<Graphics::SBs::InstanceSB>& GetParticles() const;

    void SetFollowOwner(const bool follow);
    void SetCount(const size_t count);
    void SetDuration(const float duration);
    void SetSize(const float size);
    void SetComputeShader(const WeakComputeShader& cs);

    void LinearSpread(const Vector3& local_min, const Vector3& local_max);

    bool IsFollowOwner() const;

  private:
    SERIALIZER_ACCESS
    ParticleRenderer();

    bool  m_b_follow_owner_;
    float m_duration_dt_;
    float m_size_;

    Graphics::StructuredBuffer<Graphics::SBs::InstanceParticleSB> m_sb_buffer_;
    std::vector<Graphics::SBs::InstanceParticleSB>                m_instances_;
    std::string                                                   m_cs_name_;
    StrongComputeShader                                           m_cs_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::ParticleRenderer)

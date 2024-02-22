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
    // int
    constexpr static size_t particle_count_slot = 1;
    constexpr static size_t scaling_active_slot = 2;

    // float
    constexpr static size_t dt_slot             = 2;
    constexpr static size_t duration_slot       = 3;
    constexpr static size_t scaling_min_slot    = 4;
    constexpr static size_t scaling_max_slot    = 5;

    ParticleRenderer(const WeakObject& owner);

    void Initialize() override;
    void Update(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void OnDeserialized() override;

    const std::vector<Graphics::SBs::InstanceSB>& GetParticles() const;

    void SetFollowOwner(const bool follow);
    void SetCount(const size_t count);
    void SetDuration(const float duration);
    void SetSize(const float size);
    void SetComputeShader(const WeakComputeShader& cs);

    void SetScaling(const bool scaling);
    void SetScalingParam(const float min, const float max);

    void LinearSpread(const Vector3& local_min, const Vector3& local_max);

    bool IsFollowOwner() const;

  private:
    SERIALIZER_ACCESS
    ParticleRenderer();

    bool  m_b_follow_owner_;
    bool  m_b_scaling_;

    float m_duration_dt_;
    float m_size_;

    float m_max_scale_size_;
    float m_min_scale_size_;

    Graphics::StructuredBuffer<Graphics::SBs::InstanceParticleSB> m_sb_buffer_;
    std::vector<Graphics::SBs::InstanceParticleSB>                m_instances_;
    std::string                                                   m_cs_meta_path_str_;
    // Note that we need to store in strong sense due to the gc by the resource manager.
    StrongComputeShader                                           m_cs_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::ParticleRenderer)

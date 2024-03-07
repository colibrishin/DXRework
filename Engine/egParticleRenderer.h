#pragma once
#include "egMacro.h"
#include "egRenderComponent.h"
#include "egStructuredBuffer.hpp"

namespace Engine::Components
{
  class ParticleRenderer : public Base::RenderComponent
  {
  public:
    // int
    constexpr static size_t particle_count_slot = 1;

    // float
    constexpr static size_t duration_slot       = 2;
    constexpr static size_t size_slot           = 3;

    RENDER_COM_T(RENDER_COM_T_PARTICLE)

    ParticleRenderer(const WeakObject& owner);

    void Initialize() override;
    void Update(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;

    void OnSerialized() override;
    void OnDeserialized() override;
    void OnImGui() override;

    const std::vector<Graphics::SBs::InstanceSB>& GetParticles() const;

    void SetFollowOwner(const bool follow);
    void SetCount(const size_t count);
    void SetDuration(const float duration);
    void SetSize(const float size);
    void SetComputeShader(const WeakComputeShader& cs);

    bool IsFollowOwner() const;

  private:
    SERIALIZER_ACCESS
    friend class Resources::ComputeShader;
    ParticleRenderer();

    bool  m_b_follow_owner_;

    Graphics::ParamBase                                           m_params_;
    Graphics::StructuredBuffer<Graphics::SBs::InstanceParticleSB> m_sb_buffer_;
    InstanceParticles                                             m_instances_;

    std::string                                                   m_cs_meta_path_str_;
    // Note that we need to store in strong sense due to the gc by the resource manager.
    StrongComputeShader                                           m_cs_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::ParticleRenderer)

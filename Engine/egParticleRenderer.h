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
    constexpr static size_t particle_count_slot = 0;

    // float
    constexpr static size_t duration_slot       = 0;
    constexpr static size_t size_slot           = 1;

    RENDER_COM_T(RENDER_COM_T_PARTICLE)

    ParticleRenderer(const WeakObjectBase& owner);

    ParticleRenderer(const ParticleRenderer& other);
    ParticleRenderer& operator=(const ParticleRenderer& other);

    ParticleRenderer(ParticleRenderer&& other) noexcept = delete;
    ParticleRenderer& operator=(ParticleRenderer&& other) noexcept = delete;

    void Initialize() override;
    void Update(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;

    void OnSerialized() override;
    void OnDeserialized() override;
    void OnImGui() override;

    [[nodiscard]] std::vector<Graphics::SBs::InstanceSB> GetParticles();

    void SetFollowOwner(const bool follow);
    void SetCount(const size_t count);
    void SetDuration(const float duration);
    void SetSize(const float size);
    void SetComputeShader(const WeakComputeShader& cs);

    bool IsFollowOwner() const;

  private:
    SERIALIZE_DECL
    COMP_CLONE_DECL

    friend class Resources::ComputeShader;
    ParticleRenderer();

    bool  m_b_follow_owner_;

    Graphics::SBs::LocalParamSB                                   m_params_;
    Graphics::StructuredBuffer<Graphics::SBs::LocalParamSB>       m_local_param_buffer_;
    Graphics::StructuredBuffer<Graphics::SBs::InstanceParticleSB> m_sb_buffer_;

    std::mutex                                                    m_instances_mutex_;
    InstanceParticles                                             m_instances_;

    std::string                                                   m_cs_meta_path_str_;
    // Note that we need to store in strong sense due to the gc by the resource manager.
    StrongComputeShader                                           m_cs_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::ParticleRenderer)

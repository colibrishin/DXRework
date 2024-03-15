#pragma once
#include "egManager.hpp"
#include "egMaterial.h"
#include "egModelRenderer.h"

namespace Engine::Manager::Graphics
{
  class Renderer : public Abstract::Singleton<Renderer>
  {
  private:
    using CandidatePair = std::pair<StrongObjectBase, std::vector<SBs::InstanceSB>>;

    template <typename T>
    using MaterialMap = std::map<StrongMaterial, std::vector<T>>;

    template <typename T>
    using RenderMap = std::map<eRenderComponentType, MaterialMap<T>>;

  public:
    explicit Renderer(SINGLETON_LOCK_TOKEN)
      : Singleton(),
        m_b_ready_(false) {}

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void Initialize() override;

    bool Ready() const;
    void RenderPass(
      float                                           dt,
      eShaderDomain                                   domain,
      bool                                            shader_bypass,
      const std::function<bool(const StrongObjectBase&)>& predicate
    ) const;

  private:
    friend struct SingletonDeleter;
    ~Renderer() override = default;

    void renderPassImpl(
      const float            dt,
      eShaderDomain          domain,
      bool                   shader_bypass,
      const StrongMaterial & material, const std::vector<SBs::InstanceSB> & structured_buffers
    ) const;

    void preMappingModel(const StrongRenderComponent& rc);
    void preMappingParticle(const StrongRenderComponent& rc);

    bool m_b_ready_;

    RenderMap<CandidatePair> m_render_candidates_[SHADER_DOMAIN_MAX];
  };
}

#pragma once
#include "egManager.hpp"
#include "egModelRenderer.h"

namespace Engine::Manager::Graphics
{
  class Renderer : public Abstract::Singleton<Renderer>
  {
  public:
    template <typename T>
    using MaterialMap = std::map<WeakMaterial, T, WeakComparer<Resources::Material>>;

    template <typename T>
    using RenderComponentMap = std::map<const eRenderComponentType, MaterialMap<T>>;

    template <typename T>
    using RenderPassMap = std::map<const eShaderDomain, RenderComponentMap<T>>;

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
      const float   dt,
      eShaderDomain domain,
      bool          shader_bypass = false, const std::function<bool(const StrongObject&)> & predicate = nullptr
    ) const;

  private:
    friend struct SingletonDeleter;
    ~Renderer() override = default;

    void renderPassImpl(
      const float            dt,
      eShaderDomain          domain,
      bool                   shader_bypass,
      UINT                   instance_count,
      const StrongMaterial & material, const std::vector<SBs::InstanceSB> & structured_buffers
    ) const;

    void preMappingModel(const StrongRenderComponent& rc);

    bool m_b_ready_;

    RenderPassMap<std::vector<WeakObject>> m_render_passes_;
    RenderPassMap<std::vector<SBs::InstanceSB>> m_sbs_;
  };
}

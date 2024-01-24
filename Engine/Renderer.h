#pragma once
#include "egManager.hpp"
#include "egModelRenderer.h"

namespace Engine::Manager::Graphics
{
  class Renderer : public Abstract::Singleton<Renderer>
  {
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
    void RenderPass(const float dt, const std::function<bool(const StrongObject&)>& predicate, bool post = false, bool shader_bypass = false) const;
    void RenderPass(const float dt, bool post = false, bool shader_bypass = false) const;

  private:
    friend struct SingletonDeleter;
    ~Renderer() override = default;

    void DoRenderPass(
      const float                         dt,
      bool                                shader_bypass,
      UINT                                instance_count,
      const StrongMaterial&               material,
      const std::vector<SBs::InstanceSB>& structured_buffers
    ) const;

    bool m_b_ready_;

    std::map<StrongMaterial, std::vector<StrongModelRenderer>> m_normal_passes_;
    std::map<StrongMaterial, std::vector<StrongModelRenderer>> m_post_passes_;

    std::map<StrongMaterial, std::vector<SBs::InstanceSB>>  m_normal_sbs_;
    std::map<StrongMaterial, std::vector<SBs::InstanceSB>>  m_post_sbs_;
  };
}

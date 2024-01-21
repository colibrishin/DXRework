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

    [[nodiscard]] bool Ready() const;
    void RenderPass(const float & dt, bool post = false, bool shader = true);

  private:
    using ObjectInstancingMap = std::map<WeakObject, SBs::InstanceSB, WeakComparer<Abstract::Object>>;
    using SkinnedInstancingMap = std::map<WeakObject, std::vector<Matrix>, WeakComparer<Abstract::Object>>;

    struct AnimationTexture
    {
      ComPtr<ID3D11Texture2D> texture;
      ComPtr<ID3D11ShaderResourceView> srv;
    };

    friend struct SingletonDeleter;
    ~Renderer() override = default;

    bool m_b_ready_;

    std::map<WeakMaterial, ObjectInstancingMap, WeakComparer<Resources::Material>> m_normal_instance_map_;
    std::map<WeakMaterial, SkinnedInstancingMap, WeakComparer<Resources::Material>> m_skinned_instance_map_;
    std::map<WeakMaterial, ObjectInstancingMap, WeakComparer<Resources::Material>> m_delayed_instance_map_;
    std::map<WeakMaterial, AnimationTexture, WeakComparer<Resources::Material>> m_animation_texture_map_;

    std::queue<WeakObject> m_delayed_objects_;
  };
}

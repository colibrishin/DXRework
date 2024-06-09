#pragma once
#include "egManager.hpp"
#include "egStructuredBuffer.hpp"
#include "egShadowTexture.h"

namespace Engine::Manager::Graphics
{
  constexpr float placeholder = 0.f;
  using namespace Engine::Graphics;

  class ShadowManager : public Abstract::Singleton<ShadowManager>
  {
  private:
    struct Subfrusta
    {
      Vector4 corners[8];
    };

  public:
    explicit ShadowManager(SINGLETON_LOCK_TOKEN)
      : Singleton<ShadowManager>(),
        m_viewport_(),
        m_shadow_map_mask_("", {}) {}

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void Reset();
    void RegisterLight(const WeakLight& light);
    void UnregisterLight(const WeakLight& light);

    static void EvalShadowVP(const WeakCamera & ptr_cam, const Vector3 & light_dir, SBs::LightVPSB & buffer);

    void BindShadowMaps(const CommandPair & cmd, const DescriptorPtr & heap) const;
    void UnbindShadowMaps(const CommandPair& cmd) const;

  private:
    friend struct SingletonDeleter;
    ~ShadowManager() override;

    void InitializeViewport();
    void InitializeProcessor();
    void InitializeShadowBuffer(const LocalActorID id);

    void BuildShadowMap(const float dt, const StrongLight & light) const;
    void ClearShadowMaps();

    static void CreateSubfrusta(
      const Matrix& projection, float start, float end,
      Subfrusta&    subfrusta
    );

    StrongShader m_shadow_shader_;

    // sub part of the view frustum
    Subfrusta m_subfrusta_[3];

    // lights from current scene
    std::map<LocalActorID, WeakLight> m_lights_;

    // The DX resources for each of the shadow map (texture, depth stencil view and shader resource view)
    std::map<LocalActorID, Resources::ShadowTexture> m_shadow_texs_;

    // light structured buffer
    Strong<StructuredBuffer<SBs::LightSB>> m_sb_light_buffer_{};
    // The structured buffer for the chunk of light view projection matrices
    Strong<StructuredBuffer<SBs::LightVPSB>> m_sb_light_vps_buffer_;

    D3D12_VIEWPORT m_viewport_;
    D3D12_RECT     m_scissor_rect_;

    Resources::Texture2D m_shadow_map_mask_;

    ComPtr<ID3D12DescriptorHeap> m_sampler_heap_;
  };
} // namespace Engine::Manager::Graphics

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
        m_shadow_map_mask_("", {}),
        m_current_shadow_maps_{} {}

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

  private:
    friend struct SingletonDeleter;
    ~ShadowManager() override;

    void InitializeViewport();
    void InitializeProcessor();
    void InitializeShadowBuffer(const LocalActorID id);

    void BuildShadowMap(const float dt);
    void ClearShadowMaps();

    static void CreateSubfrusta(
      const Matrix& projection, float start, float end,
      Subfrusta&    subfrusta
    );

    StrongMaterial m_shadow_shaders_;

    // sub part of the view frustum
    Subfrusta m_subfrusta_[3];

    // lights from current scene
    std::map<LocalActorID, WeakLight> m_lights_;

    // The DX resources for each of the shadow map (texture, depth stencil view and shader resource view)
    std::map<LocalActorID, Resources::ShadowTexture> m_shadow_texs_;

    // light structured buffer
    StructuredBuffer<SBs::LightSB> m_sb_light_buffer_{};
    // The structured buffer for the chunk of light view projection matrices
    StructuredBuffer<SBs::LightVPSB> m_sb_light_vps_buffer_;

    D3D11_VIEWPORT m_viewport_;
    Resources::Texture2D m_shadow_map_mask_;

    // todo: refactoring
    ID3D11ShaderResourceView*  m_current_shadow_maps_[g_max_lights];
    ComPtr<ID3D11SamplerState> m_shadow_sampler_;
  };
} // namespace Engine::Manager::Graphics

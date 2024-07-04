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
        m_scissor_rect_(),
        m_shadow_map_mask_("", {}) {}

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void GetLightVP(const boost::shared_ptr<Scene>& scene, std::vector<SBs::LightVPSB>& current_light_vp);
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void Reset();
    void RegisterLight(const WeakLight& light);
    void UnregisterLight(const WeakLight& light);

    static void EvalShadowVP(const WeakCamera & ptr_cam, const Vector3 & light_dir, SBs::LightVPSB & buffer);

    void BindShadowMaps(const Weak<CommandPair> & cmd, const DescriptorPtr & heap) const;
    void BindShadowSampler(const DescriptorPtr& heap) const;
    void UnbindShadowMaps(const Weak<CommandPair> & w_cmd) const;

    StructuredBuffer<SBs::LightSB>*   GetLightBuffer();
    StructuredBuffer<SBs::LightVPSB>* GetLightVPBuffer();

  private:
    friend struct SingletonDeleter;
    ~ShadowManager() override;

    void InitializeViewport();
    void InitializeProcessor();
    void InitializeShadowBuffer(const LocalActorID id);

    void BuildShadowMap(
      const float dt, const Weak<CommandPair> & w_cmd, const StrongLight & light, const UINT
      light_idx
    );
    void   ClearShadowMaps(const Weak<CommandPair> & w_cmd);

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
    StructuredBuffer<SBs::LightSB> m_sb_light_buffer_{};
    // The structured buffer for the chunk of light view projection matrices
    StructuredBuffer<SBs::LightVPSB> m_sb_light_vps_buffer_;

    

    D3D12_VIEWPORT m_viewport_;
    D3D12_RECT     m_scissor_rect_;

    DescriptorContainer                         m_shadow_descriptor_heap_;

    StructuredBufferMemoryPool<SBs::InstanceSB> m_shadow_instance_buffer_;
    StructuredBufferMemoryPool<SBs::LocalParamSB> m_local_param_buffers_;

    Resources::Texture2D m_shadow_map_mask_;

    ComPtr<ID3D12DescriptorHeap> m_sampler_heap_;
  };
} // namespace Engine::Manager::Graphics


REGISTER_TYPE(Engine::Manager::Application, Engine::Manager::Graphics::ShadowManager)
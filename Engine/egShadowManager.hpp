#pragma once
#include "egManager.hpp"

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
        ShadowManager(SINGLETON_LOCK_TOKEN)
        : Singleton<ShadowManager>() {}

        ~ShadowManager() override;
        void InitializeProcessors();
        void InitializeViewport();
        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostUpdate(const float& dt) override;

        void                        Reset();
        void                        RegisterLight(const WeakLight& light);
        void                        UnregisterLight(const WeakLight& light);

    private:
        void InitializeShadowBuffer(DXPacked::ShadowVPResource& buffer);
        void BuildShadowMap(Scene& scene, const float dt) const;
        void ClearShadowVP();
        void ClearShadowMaps();

        void CreateSubfrusta(
            const Matrix& projection, float start, float end,
            Subfrusta&    subfrusta) const;
        void EvalShadowVP(
            const Vector3& light_dir, CBs::ShadowVPCB&        buffer,
            UINT           light_index, CBs::ShadowVPChunkCB& chunk);

        boost::shared_ptr<Resources::VertexShader>   m_vs_stage1;
        boost::shared_ptr<Resources::GeometryShader> m_gs_stage1;
        boost::shared_ptr<Resources::PixelShader>    m_ps_stage1;

        // sub part of the view frustum
        Subfrusta m_subfrusta_[3];

        // lights from current scene
        std::set<WeakLight, WeakComparer<Objects::Light>> m_lights_;

        // light constant buffer
        CBs::LightCB m_light_buffer_{};

        // The DX resources for each of the shadow map (texture, depth stencil view and shader resource view)
        std::map<WeakLight, DXPacked::ShadowVPResource, WeakComparer<Objects::Light>>
        m_dx_resource_shadow_vps_;
        // The constant buffer for each of the shadow map
        std::map<WeakLight, CBs::ShadowVPCB, WeakComparer<Objects::Light>>
        m_cb_shadow_vps_;

        // The constant buffer for the chunk of shadow map
        CBs::ShadowVPChunkCB m_cb_shadow_vps_chunk_;

        D3D11_VIEWPORT m_viewport_;

        ID3D11ShaderResourceView* m_current_shadow_maps_[g_max_lights];

        ComPtr<ID3D11SamplerState>      m_shadow_map_sampler_state_;
        ComPtr<ID3D11DepthStencilState> m_shadow_map_depth_stencil_state_;

    };
} // namespace Engine::Manager::Graphics

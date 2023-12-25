#pragma once
#include "egManager.hpp"

namespace Engine::Manager::Graphics
{
    constexpr float placeholder = 0.f;

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

        ~ShadowManager() override = default;
        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostUpdate(const float& dt) override;

        void Clear();
        void RegisterLight(const WeakLight& light);
        void UnregisterLight(const WeakLight& light);

    private:
        void CreateSubfrusta(
            const Matrix& projection, float start, float end,
            Subfrusta&    subfrusta) const;
        void EvalCascadeVP(
            const Vector3& light_dir, CascadeShadowBuffer&        buffer,
            UINT           light_index, CascadeShadowBufferChunk& chunk);

        void BuildShadowMap(Scene& scene) const;
        void BindShadowMapChunk();
        void ClearShadowBufferChunk();
        void ClearShadowMaps();

    private:
        boost::shared_ptr<Graphic::VertexShader>   m_vs_stage1;
        boost::shared_ptr<Graphic::GeometryShader> m_gs_stage1;
        boost::shared_ptr<Graphic::PixelShader>    m_ps_stage1;

        Subfrusta m_subfrusta_[3];

        std::set<WeakLight, WeakComparer<Objects::Light>> m_lights_;
        std::map<WeakLight, GraphicShadowBuffer, WeakComparer<Objects::Light>>
        m_graphic_shadow_buffer_;
        std::map<WeakLight, CascadeShadowBuffer, WeakComparer<Objects::Light>>
        m_cascade_vp_buffer_;

        CascadeShadowBufferChunk m_cascade_shadow_buffer_chunk_;
    };
} // namespace Engine::Manager::Graphics

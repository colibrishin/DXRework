#pragma once
#include "egComponent.h"

namespace Engine::Components
{
    class ModelRenderer final : public Abstract::Component
	{
    public:
        INTERNAL_COMP_CHECK_CONSTEXPR(COM_T_MODEL_RENDERER)

        ModelRenderer(const WeakObject& owner);
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void PostRender(const float& dt) override;
        void Render(const float& dt) override;
        void RenderMeshOnly(const float& dt);

        void SetModel(const WeakModel & model);
        void AddVertexShader(const WeakVertexShader & vertex_shader);
        void AddPixelShader(const WeakPixelShader & pixel_shader);

    private:
        std::vector<StrongVertexShader> m_vertex_shaders_;
        std::vector<StrongPixelShader> m_pixel_shaders_;

        StrongModel m_model_;
    };
}
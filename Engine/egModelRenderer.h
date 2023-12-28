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
        void PostUpdate(const float& dt) override;

        void SetShape(const WeakModel& model);
        void SetMaterial(const WeakMaterial& material);
        WeakModel GetModel() const;

    private:
        friend class Manager::Graphics::Renderer;

        StrongMaterial m_material_;
        StrongModel m_model_;
    };
}
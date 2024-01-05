#pragma once
#include "egComponent.h"

namespace Engine::Components
{
    class ModelRenderer final : public Abstract::Component
	{
    public:
        COMPONENT_T(COM_T_MODEL_RENDERER)

        ModelRenderer(const WeakObject& owner);
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostUpdate(const float& dt) override;

        void SetShape(const WeakModel& model);
        void SetMaterial(const WeakMaterial& material);
        WeakModel GetModel() const;
        WeakMaterial GetMaterial() const;

    private:
        SERIALIZER_ACCESS
        ModelRenderer();

        friend class Manager::Graphics::Renderer;

        std::string m_model_name_;
        std::string m_material_name_;

        StrongMaterial m_material_;
        StrongModel m_model_;
    };
}
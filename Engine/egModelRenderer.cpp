#include "pch.h"
#include "egModelRenderer.h"

#include "egDebugger.hpp"
#include "egShape.h"

namespace Engine::Components
{
    ModelRenderer::ModelRenderer(const WeakObject& owner) : Component(COM_T_MODEL_RENDERER, owner) {}

    void ModelRenderer::PreUpdate(const float& dt) {}

    void ModelRenderer::Update(const float& dt) {}

    void ModelRenderer::FixedUpdate(const float& dt) {}

    void ModelRenderer::PostUpdate(const float& dt)
    {
        Component::PostUpdate(dt);
    }

    void ModelRenderer::SetShape(const WeakModel& model)
    {
        if (const auto m = model.lock())
        {
            m_model_ = m;
        }
    }

    void ModelRenderer::SetMaterial(const WeakMaterial& material)
    {
        if (const auto m = material.lock())
        {
            m_material_ = m;
        }
    }

    WeakModel ModelRenderer::GetModel() const
    {
        return m_model_;
    }
}

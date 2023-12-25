#include "pch.h"
#include "egModelRenderer.h"

#include "egDebugger.hpp"
#include "egModel.h"

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

    void ModelRenderer::SetModel(const WeakModel& model)
    {
        if (const auto m = model.lock())
        {
            m_model_ = m;
        }
    }

    void ModelRenderer::AddVertexShader(const WeakVertexShader& vertex_shader)
    {
        if (const auto vtx = vertex_shader.lock())
        {
            m_vertex_shaders_.push_back(vtx);
        }
    }

    void ModelRenderer::AddPixelShader(const WeakPixelShader& pixel_shader)
    {
        if (const auto pix = pixel_shader.lock())
        {
            m_pixel_shaders_.push_back(pix);
        }
    }

    WeakModel ModelRenderer::GetModel() const
    {
        return m_model_;
    }
}

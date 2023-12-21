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

    void ModelRenderer::PreRender(const float& dt) {}

    void ModelRenderer::PostRender(const float& dt) {}

    void ModelRenderer::Render(const float& dt)
    {
        Component::Render(dt);

        if (m_vertex_shaders_.empty())
        {
            GetDebugger().Log(L"ModelRenderer::Render() : Vertex shader is null");
            return;
        }

        if (m_pixel_shaders_.empty())
        {
            GetDebugger().Log(L"ModelRenderer::Render() : Pixel shader is null");
            return;
        }

        const auto render_targets = m_model_->GetRemainingRenderIndex();

        for (auto i = 0; i < render_targets; ++i)
        {
            const auto vtx = m_vertex_shaders_[i % m_vertex_shaders_.size()];
            const auto pix = m_pixel_shaders_[i % m_pixel_shaders_.size()];

            vtx->Render(dt);
            pix->Render(dt);

            m_model_->Render(dt);

            GetRenderPipeline().ResetShaders();
        }

        m_model_->ResetRenderIndex();
    }

    void ModelRenderer::RenderMeshOnly(const float& dt)
    {
        Component::Render(dt);

        const auto render_targets = m_model_->GetRemainingRenderIndex();

        for (int i = 0; i < render_targets; ++i)
        {
            m_model_->RenderMeshOnly(dt);
        }

        m_model_->ResetRenderIndex();
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
}

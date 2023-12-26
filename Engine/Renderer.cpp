#include "pch.h"
#include "Renderer.h"

#include "egAnimation.h"
#include "egAnimator.h"
#include "egModel.h"
#include "egModelRenderer.h"
#include "egNormalMap.h"
#include "egProjectionFrustum.h"
#include "egReflectionEvaluator.h"
#include "egSceneManager.hpp"
#include "egTexture.h"
#include "egTransform.h"

namespace Engine::Manager::Graphics
{
    void Renderer::PreUpdate(const float& dt) {}

    void Renderer::Update(const float& dt) {}

    void Renderer::FixedUpdate(const float& dt) {}

    void Renderer::PreRender(const float& dt) {}

    void Renderer::RenderModel(const float& dt, const WeakModelRenderer& ptr_mr, const WeakTransform& ptr_tr, const WeakAnimator& ptr_atr)
    {
        const auto mr = ptr_mr.lock();
        const auto tr = ptr_tr.lock();

        if (mr->m_vertex_shaders_.empty())
        {
            GetDebugger().Log(L"ModelRenderer::Render() : Vertex shader is null");
            return;
        }

        if (mr->m_pixel_shaders_.empty())
        {
            GetDebugger().Log(L"ModelRenderer::Render() : Pixel shader is null");
            return;
        }

        const auto& model      = mr->m_model_;
        const auto  mesh_count = model->GetMeshCount();

        Components::Transform::Bind(*tr);

        for (auto i = 0; i < mesh_count; ++i)
        {
            const auto vtx = mr->m_vertex_shaders_[i % mr->m_vertex_shaders_.size()];
            const auto pix = mr->m_pixel_shaders_[i % mr->m_pixel_shaders_.size()];

            vtx->Render(dt);
            pix->Render(dt);

            const auto& mesh = model->GetMesh(i);

            if (mesh.expired()) continue;

            if (!model->m_textures_.empty()) model->m_textures_[i]->Render(dt);
            if (!model->m_normal_maps_.empty()) model->m_normal_maps_[i]->Render(dt);

            if (const auto atr = ptr_atr.lock())
            {
                const auto ptr_anim = atr->GetAnimation();

                if (const auto anim = ptr_anim.lock())
                {
                    anim->Render(dt);
                }
            }
            
            mesh.lock()->Render(dt);

            GetRenderPipeline().ResetShaders();
            GetRenderPipeline().UnbindResource(SR_NORMAL_MAP);
            GetRenderPipeline().UnbindResource(SR_TEXTURE);
            GetRenderPipeline().UnbindResource(SR_ANIMATION, SHADER_VERTEX);
        }
    }

    void Renderer::Render(const float& dt)
    {
        const auto& scene = GetSceneManager().GetActiveScene().lock();

        for (const auto& [type, layer] : *scene)
        {
            if (type == LAYER_UI)
            {
                GetReflectionEvaluator().RenderFinished();
            }

            for (const auto& object : *layer)
            {
                if (!object->GetActive())
                {
                    continue; 
                }

                if (!GetProjectionFrustum().CheckRender(object))
                {
                    continue;
                }

                if (object->GetObjectType() == DEF_OBJ_T_DELAY_OBJ)
                {
                    m_delayed_objects_.push(object);
                    continue;
                }

                const auto ptr_mr = object->GetComponent<Components::ModelRenderer>();
                const auto& ptr_tr = object->GetComponent<Components::Transform>();
                const auto& ptr_atr = object->GetComponent<Components::Animator>();

                if (ptr_mr.expired()) continue;
                if (ptr_tr.expired()) continue;

                RenderModel(dt, ptr_mr, ptr_tr, ptr_atr);
            }
        }

        while (!m_delayed_objects_.empty())
        {
            const auto ptr_object = m_delayed_objects_.front();
            m_delayed_objects_.pop();

            if (ptr_object.expired()) continue;

            const auto object = ptr_object.lock();

            const auto  ptr_mr = object->GetComponent<Components::ModelRenderer>();
            const auto& ptr_tr = object->GetComponent<Components::Transform>();
            const auto& ptr_atr = object->GetComponent<Components::Animator>();

            if (ptr_mr.expired()) continue;
            if (ptr_tr.expired()) continue;

            RenderModel(dt, ptr_mr, ptr_tr, ptr_atr);
        }
    }

    void Renderer::PostRender(const float& dt) {}

    void Renderer::PostUpdate(const float& dt) {}

    void Renderer::Initialize() {}
}

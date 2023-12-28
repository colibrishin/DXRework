#include "pch.h"
#include "Renderer.h"

#include "egBaseAnimation.h"
#include "egAnimator.h"
#include "egBoneAnimation.h"
#include "egMaterial.h"
#include "egShape.h"
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

        const auto& ptr_model = mr->GetModel();
        const auto& ptr_mtr = mr->GetMaterial();

        if (ptr_model.expired()) return;
        if (ptr_mtr.expired()) return;

        const auto& model = ptr_model.lock();
        const auto& mtr = ptr_mtr.lock();

        if (const auto atr = ptr_atr.lock())
        {
	        const auto ptr_anim = atr->GetAnimation();

	        if (const auto anim = ptr_anim.lock())
	        {
		        anim->PreRender(atr->GetFrame());
                anim->Render(atr->GetFrame());
	        }
        }

    	Components::Transform::Bind(*tr);
        mtr->PreRender(dt);
    	mtr->Render(dt);
        model->PreRender(dt);
        model->Render(dt);
        mtr->PostRender(dt);
        model->PostRender(dt);

        if (const auto atr = ptr_atr.lock())
        {
	        const auto ptr_anim = atr->GetAnimation();

	        if (const auto anim = ptr_anim.lock())
	        {
		        anim->PostRender(atr->GetFrame());
	        }
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

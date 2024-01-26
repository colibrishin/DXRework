#include "pch.h"
#include "Renderer.h"

#include "egAnimator.h"
#include "egBaseAnimation.h"
#include "egBoneAnimation.h"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egProjectionFrustum.h"
#include "egReflectionEvaluator.h"
#include "egSceneManager.hpp"
#include "egShape.h"
#include "egTexture.h"
#include "egTransform.h"

namespace Engine::Manager::Graphics
{
  void Renderer::PreUpdate(const float& dt) {}

  void Renderer::Update(const float& dt) {}

  void Renderer::FixedUpdate(const float& dt) {}

  void Renderer::PreRender(const float& dt)
  {
    // Pre-processing, Mapping the materials to the model renderers.
    const auto& scene = GetSceneManager().GetActiveScene().lock();
    const auto& rcs = scene->GetCachedComponents<Components::Base::RenderComponent>();

    for (const auto& ptr_rc : rcs)
    {
      // pointer sanity check
      if (ptr_rc.expired()) { continue; }
      const auto rc = ptr_rc.lock()->GetSharedPtr<Components::Base::RenderComponent>();

      // owner object sanity check
      const auto obj = rc->GetOwner().lock();
      if (!obj) { continue; }
      if (!obj->GetActive()) { continue; }

      // material sanity check
      const auto ptr_mtr = rc->GetMaterial();
      if (ptr_mtr.expired()) { continue; }
      const auto mtr = ptr_mtr.lock();

      // transform check, continue if it is disabled. (undefined behaviour)
      const auto tr = obj->GetComponent<Components::Transform>().lock();
      if (!tr) { continue; }
      if (!tr->GetActive()) { continue; }

      switch (rc->GetRenderType())
      {
      case RENDER_COM_T_MODEL:
          preMappingModel(rc);
          break;
      case RENDER_COM_T_PARTICLE: 
          break;
      case RENDER_COM_T_UNK:
      default: break;
      }
    }

    m_b_ready_ = true;
  }

  void Renderer::Render(const float& dt)
  {
    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      if (const auto cam = scene->GetMainCamera().lock())
      {
        for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
        {
          // Check culling.
          RenderPass(dt, (eShaderDomain)i, false, [](const StrongObject& obj)
          {
            return GetProjectionFrustum().CheckRender(obj);
          });

          if (i == SHADER_DOMAIN_OPAQUE)
          {
            // Notify reflection evaluator that rendering is finished so that it
            // can copy the rendered scene to the copy texture.
            GetReflectionEvaluator().RenderFinished();
          }
        }
      }
    }
  }

  void Renderer::PostRender(const float& dt)
  {
    m_render_passes_.clear();
    m_sbs_.clear();
    m_b_ready_ = false;
  }

  void Renderer::PostUpdate(const float& dt) {}

  void Renderer::Initialize() {}

  bool Renderer::Ready() const { return m_b_ready_; }

  void Renderer::RenderPass(
    const float                                     dt,
    eShaderDomain                                   domain,
    bool                                            shader_bypass,
    const std::function<bool(const StrongObject&)>& predicate
  ) const
  {
    if (!Ready())
    {
      GetDebugger().Log("Renderer is not ready!");
      return;
    }

    if (!m_render_passes_.contains(domain)) { return; }

    const auto& target_map = m_render_passes_.at(domain);
    const auto& target_sb  = m_sbs_.at(domain);

    if (target_map.empty()) { return; }
    if (target_sb.empty()) { return; }

    for (const auto& [rc_type, mtr_map] : target_map)
    {
      for (const auto& [ptr_mtr, objs] : mtr_map)
      {
        if (predicate)
        {
          std::vector<WeakObject>      obj_pred_set;
          std::vector<SBs::InstanceSB> sb_pred_set;

          for (int i = 0; i < objs.size(); ++i)
          {
            const auto& obj = objs[i].lock();
            const auto& sb  = target_sb.at(rc_type).at(ptr_mtr).at(i);

            if (!predicate(obj)) { continue; }

            obj_pred_set.push_back(obj);
            sb_pred_set.push_back(sb);
          }

          if (obj_pred_set.empty()) { continue; }
          if (sb_pred_set.empty()) { continue; }

          const auto mtr = ptr_mtr.lock();
          renderPassImpl(dt, domain, shader_bypass, sb_pred_set.size(), mtr, sb_pred_set);
        }
        else
        {
          const auto& mtr_sbs = target_sb.at(rc_type).at(ptr_mtr);
          const auto mtr = ptr_mtr.lock();

          renderPassImpl(dt, domain, shader_bypass, mtr_map.at(mtr).size(), mtr, mtr_sbs);
        }
      }
    }
  }

  void Renderer::renderPassImpl(
    const float                         dt,
    eShaderDomain                       domain,
    bool                                shader_bypass,
    UINT                                instance_count,
    const StrongMaterial&               material,
    const std::vector<SBs::InstanceSB>& structured_buffers
  ) const
  {
    StructuredBuffer<SBs::InstanceSB> sb;
    sb.Create(instance_count, structured_buffers.data(), false);
    sb.BindSRV(SHADER_VERTEX);

    material->SetTempParam
      (
       {
         .instanceCount = instance_count,
         .bypassShader = shader_bypass,
         .domain = domain,
       }
      );

    material->PreRender(dt);
    material->Render(dt);
    material->PostRender(dt);

    sb.UnbindSRV(SHADER_VERTEX);
  }

  void Renderer::preMappingModel(const StrongRenderComponent& rc)
  {
    // get model renderer, continue if it is disabled
    const auto mr = rc->GetSharedPtr<Components::ModelRenderer>();
    if (!mr->GetActive()) { return; }

    const auto obj = rc->GetOwner().lock();
    const auto mtr = rc->GetMaterial().lock();
    const auto tr = obj->GetComponent<Components::Transform>().lock();

    // animator parameters
    float anim_frame    = 0.0f;
    UINT  anim_idx      = 0;
    UINT  anim_duration = 0;
    bool  no_anim       = false;

    if (const auto atr = obj->GetComponent<Components::Animator>().lock())
    {
      anim_frame = atr->GetFrame();
      anim_idx   = atr->GetAnimation();
      no_anim    = !atr->GetActive();

      if (const auto bone_anim = mtr->GetResource<Resources::BoneAnimation>(anim_idx).lock())
      {
        anim_duration = (UINT)(bone_anim->GetDuration() / g_animation_sample_rate);
      }
    }

    // Pre-mapping by the material.
    for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
    {
      const auto domain = static_cast<eShaderDomain>(i);

      if (mtr->IsRenderDomain(domain))
      {
        m_render_passes_[domain][rc->GetRenderType()][mtr].push_back(obj);

        SBs::InstanceModelSB sb{};
        sb.SetWorld(tr->GetWorldMatrix().Transpose());
        sb.SetFrame(anim_frame);
        sb.SetAnimDuration(anim_duration);
        sb.SetAnimIndex(anim_idx);
        sb.SetNoAnim(no_anim);

        // todo: stacking structured buffer data might be get large easily.
        m_sbs_[domain][rc->GetRenderType()][mtr].emplace_back(sb);
      }
    }
  }
}

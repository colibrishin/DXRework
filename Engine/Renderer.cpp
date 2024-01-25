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
    const auto& mrs = scene->GetCachedComponents<Components::ModelRenderer>();

    for (const auto& ptr_mr : mrs)
    {
      // pointer sanity check
      if (ptr_mr.expired()) { continue; }

      // get model renderer, continue if it is disabled
      const auto mr = ptr_mr.lock()->GetSharedPtr<Components::ModelRenderer>();
      if (!mr->GetActive()) { continue; }

      // owner object sanity check and culling check
      const auto obj = mr->GetOwner().lock();
      if (!obj) { continue; }
      if (!obj->GetActive()) { continue; }
      if (!GetProjectionFrustum().CheckRender(obj)) { continue; }

      // material sanity check
      const auto ptr_mtr = mr->GetMaterial();
      if (ptr_mtr.expired()) { continue; }
      const auto mtr = ptr_mtr.lock();

      // transform check, continue if it is disabled. (undefined behaviour)
      const auto tr = obj->GetComponent<Components::Transform>().lock();
      if (!tr) { continue; }
      if (!tr->GetActive()) { continue; }

      // animator parameters
      float anim_frame = 0.0f;
      UINT anim_idx = 0;
      UINT anim_duration = 0;
      bool no_anim = false;

      if (const auto atr = obj->GetComponent<Components::Animator>().lock())
      {
        anim_frame = atr->GetFrame();
        anim_idx = atr->GetAnimation();
        no_anim = !atr->GetActive();

        if (const auto bone_anim = mtr->GetResource<Resources::BoneAnimation>(anim_idx).lock())
        {
          anim_duration = (UINT)(bone_anim->GetDuration() / g_animation_sample_rate);
        }
      }

      if (mtr->IsPostProcess())
      {
        m_post_passes_[mtr].push_back(mr);

        m_post_sbs_[mtr].push_back
          (
              {
                .world = tr->GetWorldMatrix().Transpose(),
                .animFrame = anim_frame,
                .boneAnimDuration = (int)anim_duration,
                .animIndex = (int)anim_idx,
                .noAnimFlag = no_anim
              }
          );

        continue;
      }

      m_normal_passes_[mtr].push_back(mr);

      m_normal_sbs_[mtr].push_back
        (
         {
           .world = tr->GetWorldMatrix().Transpose(),
           .animFrame = anim_frame,
           .boneAnimDuration = (int)anim_duration,
           .animIndex = (int)anim_idx,
           .noAnimFlag = no_anim
         }
        );
    }

    m_b_ready_ = true;
  }

  void Renderer::Render(const float& dt)
  {
    RenderPass(dt, false, false);

    // Notify reflection evaluator that rendering is finished so that it
    // can copy the rendered scene to the copy texture.
    GetReflectionEvaluator().RenderFinished();

    RenderPass(dt, true, false);
  }

  void Renderer::PostRender(const float& dt)
  {
    m_normal_passes_.clear();
    m_post_passes_.clear();
    m_normal_sbs_.clear();
    m_post_sbs_.clear();
    m_b_ready_ = false;
  }

  void Renderer::PostUpdate(const float& dt) {}

  void Renderer::Initialize() {}

  bool Renderer::Ready() const { return m_b_ready_; }

  void Renderer::RenderPass(const float dt, bool post, bool shader_bypass) const
  {
    if (!Ready())
    {
      GetDebugger().Log("Renderer is not ready!");
      return;
    }

    const auto& target_map = post ? m_post_passes_ : m_normal_passes_;
    const auto& target_sb  = post ? m_post_sbs_ : m_normal_sbs_;

    if (target_map.empty()) { return; }
    if (target_sb.empty()) { return; }

    for (const auto& mtr : target_map | std::views::keys)
    {
      DoRenderPass(dt, shader_bypass, target_sb.at(mtr).size(), mtr, target_sb.at(mtr));
    }
  }

  void Renderer::DoRenderPass(
    const float                         dt,
    bool                                shader_bypass,
    UINT                                instance_count,
    const StrongMaterial&               material,
    const std::vector<SBs::InstanceSB>& structured_buffers
  ) const
  {
    StructuredBuffer<SBs::InstanceSB> sb;
    sb.Create(instance_count, structured_buffers.data(), false);
    sb.Bind(SHADER_VERTEX);

    material->SetTempParam
      (
       {
         .instanceCount = instance_count,
         .bypassShader = shader_bypass
       }
      );

    material->PreRender(dt);
    material->Render(dt);
    material->PostRender(dt);

    sb.Unbind(SHADER_VERTEX);
  }

  void Renderer::RenderPass(
    const float                                     dt,
    const std::function<bool(const StrongObject&)>& predicate,
    bool                                            post,
    bool                                            shader_bypass
  ) const
  {
    if (!Ready())
    {
      GetDebugger().Log("Renderer is not ready!");
      return;
    }

    const auto& target_map = post ? m_post_passes_ : m_normal_passes_;
    const auto& target_sb  = post ? m_post_sbs_ : m_normal_sbs_;

    if (target_map.empty()) { return; }
    if (target_sb.empty()) { return; }

    for (const auto& [mtr, mrs] : target_map)
    {
      if (predicate)
      {
        std::vector<StrongModelRenderer> mr_pred_set;
        std::vector<SBs::InstanceSB>     sb_pred_set;

        for (int i = 0; i < mrs.size(); ++i)
        {
          const auto& mr = mrs[i];
          const auto& sb = target_sb.at(mtr)[i];

          if (!predicate(mr->GetOwner().lock())) { continue; }

          mr_pred_set.push_back(mr);
          sb_pred_set.push_back(sb);
        }

        if (mr_pred_set.empty()) { continue; }
        if (sb_pred_set.empty()) { continue; }

        DoRenderPass(dt, shader_bypass, sb_pred_set.size(), mtr, sb_pred_set);
      }
      else
      {
        DoRenderPass(dt, shader_bypass, target_sb.at(mtr).size(), mtr, target_sb.at(mtr));
      }
    }
  }
}

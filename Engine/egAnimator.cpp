#include "pch.h"
#include "egAnimator.h"
#include "egBaseAnimation.h"
#include "egBoneAnimation.h"
#include "egImGuiHeler.hpp"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egShape.h"
#include "egTransform.h"
#include "imgui_stdlib.h"

SERIALIZER_ACCESS_IMPL
(
 Engine::Components::Animator,
 _ARTAG(_BSTSUPER(Engine::Abstract::Component))
)

namespace Engine::Components
{
  Animator::Animator(const WeakObject& owner)
    : Component(COM_T_ANIMATOR, owner),
      m_animation_id_(0),
      m_current_frame_(0) {}

  void Animator::PreUpdate(const float& dt) {}

  void Animator::Update(const float& dt)
  {
    if (!GetActive()) { return; }

    const auto mr = GetOwner().lock()->GetComponent<ModelRenderer>();

    if (mr.expired()) { return; }
    const auto mat = mr.lock()->GetMaterial();
    if (mat.expired()) { return; }

    // Animator assumes that assigned material has animation.
    const auto  tr_anim   = mat.lock()->GetResource<Resources::BaseAnimation>(m_animation_id_).lock();
    const auto  bone_anim = mat.lock()->GetResource<Resources::BoneAnimation>(m_animation_id_).lock();
    const float duration  = tr_anim ? tr_anim->GetDuration() : bone_anim->GetDuration();

    m_current_frame_ += dt;

    if (tr_anim)
    {
      if (tr_anim->ConvertDtToFrame
          (m_current_frame_, tr_anim->GetTicksPerSecond(), duration) >= duration)
      {
        m_current_frame_ = 0.0f;
      }

      if (const auto tr = GetOwner().lock()->GetComponent<Transform>().lock()) { UpdateTransform(tr, tr_anim); }
    }
    else if (bone_anim)
    {
      const auto frame_t = bone_anim->ConvertDtToFrame
          (m_current_frame_, bone_anim->GetTicksPerSecond(), duration);
      if (frame_t >= duration)
      {
        m_current_frame_ = 0.0f;
      }
    }
  }

  void Animator::FixedUpdate(const float& dt) {}

  void Animator::OnImGui()
  {
    Component::OnImGui();
    lldDisabled("Animation", m_animation_id_);
    FloatDisabled("Frame", m_current_frame_);
  }

  void Animator::SetAnimation(UINT idx) { m_animation_id_ = idx; }

  UINT Animator::GetAnimation() const { return m_animation_id_; }

  float Animator::GetFrame() const { return m_current_frame_; }

  Animator::Animator()
    : Component(COM_T_ANIMATOR, {}),
      m_animation_id_(0),
      m_current_frame_(0) {}

  void Animator::UpdateTransform(const StrongTransform& tr, const StrongBaseAnimation& anim) const
  {
    if (anim)
    {
      const auto time = Resources::BaseAnimation::ConvertDtToFrame
        (
         m_current_frame_,
         anim->GetTicksPerSecond(),
         anim->GetDuration()
        );

      const auto& primitive = anim->m_simple_primitive_;

      const auto pos   = primitive.GetPosition(time);
      const auto rot   = primitive.GetRotation(time);
      const auto scale = primitive.GetScale(time);

      tr->SetAnimationPosition(pos);
      tr->SetAnimationRotation(rot);
      tr->SetAnimationScale(scale);
    }
    else
    {
      tr->SetAnimationPosition(Vector3::Zero);
      tr->SetAnimationRotation(Quaternion::Identity);
      tr->SetAnimationScale(Vector3::One);
    }
  }
}

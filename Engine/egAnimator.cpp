#include "pch.h"
#include "egAnimator.h"
#include "egShape.h"
#include "egBoneAnimation.h"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Components::Animator,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Component))
                       _ARTAG(m_animation_))

namespace Engine::Components
{
    Animator::Animator(const WeakObject& owner)
    : Component(COM_T_ANIMATOR, owner),
      m_current_frame_(0) {}

    void Animator::PreUpdate(const float& dt) {}

    void Animator::Update(const float& dt)
    {
        if (m_animation_)
        {
            if (m_current_frame_ >= m_animation_->GetDuration())
            {
                m_current_frame_ = 0.0f;
            }

            m_current_frame_ += dt;

            if (const auto tr = GetOwner().lock()->GetComponent<Transform>().lock())
            {
                UpdateTransform(tr);
            }
        }
    }

    void Animator::FixedUpdate(const float& dt) {}

    void Animator::SetAnimation(const WeakBaseAnimation& anim)
    {
        if (const auto locked = anim.lock())
        {
            m_animation_ = locked;
        }
    }

    WeakBaseAnimation Animator::GetAnimation() const
    {
        return m_animation_;
    }

    float Animator::GetFrame() const
    {
        return m_current_frame_;
    }

    Animator::Animator()
    : Component(COM_T_ANIMATOR, {}),
      m_current_frame_(0) {}

    void Animator::UpdateTransform(const StrongTransform& tr) const
    {
        if (m_animation_ && m_animation_->GetResourceType() == RES_T_BASE_ANIM)
        {
            const auto time = Resources::BaseAnimation::ConvertDtToFrame(
                                                                         m_current_frame_,
                                                                         m_animation_->GetTicksPerSecond(),
                                                                         m_animation_->GetDuration());

            const auto primitive = m_animation_->m_simple_primitive_;

            const auto pos = primitive.GetPosition(time);
            const auto rot = primitive.GetRotation(time);
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

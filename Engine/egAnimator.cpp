#include "pch.h"
#include "egAnimator.h"
#include "egShape.h"
#include "egBaseAnimation.h"
#include "egBoneAnimation.h"
#include "egImGuiHeler.hpp"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egTransform.h"
#include "imgui_stdlib.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Components::Animator,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Component)))

namespace Engine::Components
{
    Animator::Animator(const WeakObject& owner)
    : Component(COM_T_ANIMATOR, owner),
      m_current_frame_(0) {}

    void Animator::PreUpdate(const float& dt) {}

    void Animator::Update(const float& dt)
    {
        if (!GetActive()) return;

        const auto mr = GetOwner().lock()->GetComponent<ModelRenderer>();

        if (mr.expired()) return;
        const auto mat = mr.lock()->GetMaterial();
        if (mat.expired()) return;

        // Animator assumes that assigned material has animation.
        const auto tr_anim = mat.lock()->GetResource<Resources::BaseAnimation>(m_animation_name_).lock();
        const auto bone_anim = mat.lock()->GetResource<Resources::BoneAnimation>(m_animation_name_).lock();
        const float duration = tr_anim ? tr_anim->GetDuration() : bone_anim->GetDuration();

        if (tr_anim || bone_anim)
        {
            if (m_current_frame_ >= duration)
            {
                m_current_frame_ = 0.0f;
            }

            m_current_frame_ += dt;

            if (const auto tr = GetOwner().lock()->GetComponent<Transform>().lock())
            {
                UpdateTransform(tr, tr_anim);
            }
        }
    }

    void Animator::FixedUpdate(const float& dt) {}

    void Animator::OnImGui()
    {
        Component::OnImGui();
        TextDisabled("Animation", m_animation_name_);
        FloatDisabled("Frame", m_current_frame_);
    }

    void Animator::SetAnimation(const std::string& name)
    {
        m_animation_name_ = name;
    }

    std::string Animator::GetAnimation() const
    {
        return m_animation_name_;
    }

    float Animator::GetFrame() const
    {
        return m_current_frame_;
    }

    Animator::Animator()
    : Component(COM_T_ANIMATOR, {}),
      m_current_frame_(0) {}

    void Animator::UpdateTransform(const StrongTransform& tr, const StrongBaseAnimation& anim) const
    {
        if (anim)
        {
            const auto time = Resources::BaseAnimation::ConvertDtToFrame(
                                                                         m_current_frame_,
                                                                         anim->GetTicksPerSecond(),
                                                                         anim->GetDuration());

            const auto& primitive = anim->m_simple_primitive_;

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

#include "pch.h"
#include "egAnimator.h"
#include "egModel.h"
#include "egAnimation.h"

SERIALIZER_ACCESS_IMPL(Engine::Components::Animator,
_ARTAG(_BSTSUPER(Engine::Abstract::Component))
_ARTAG(m_animation_))

namespace Engine::Components
{
    Animator::Animator(const WeakObject& owner) : Component(COM_T_ANIMATOR, owner) {}

    void Animator::PreUpdate(const float& dt) {}

    void Animator::Update(const float& dt) {}

    void Animator::FixedUpdate(const float& dt) {}

    void Animator::SetAnimation(const WeakAnimation& anim)
    {
        if (const auto locked = anim.lock())
        {
            m_animation_ = locked;
        }
    }

    WeakAnimation Animator::GetAnimation() const
    {
        return m_animation_;
    }

    Animator::Animator() : Component(COM_T_ANIMATOR, {}) {}
}

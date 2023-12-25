#include "pch.h"
#include "egAnimator.h"
#include "egModel.h"

SERIALIZER_ACCESS_IMPL(Engine::Components::Animator,
_ARTAG(_BSTSUPER(Engine::Abstract::Component))
_ARTAG(m_model_)
_ARTAG(m_current_animation_))

namespace Engine::Components
{
    Animator::Animator(const WeakObject& owner) : Component(COM_T_ANIMATOR, owner) {}

    void Animator::PreUpdate(const float& dt) {}

    void Animator::Update(const float& dt) {}

    void Animator::FixedUpdate(const float& dt) {}

    void Animator::SetModel(const WeakModel& model)
    {
        if (const auto locked = model.lock())
        {
            m_model_ = locked;
        }
    }

    void Animator::SetAnimation(const std::string& animation_name)
    {
        m_current_animation_ = animation_name;
    }

    Animator::Animator() : Component(COM_T_ANIMATOR, {}) {}
}

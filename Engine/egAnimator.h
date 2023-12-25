#pragma once
#include "egComponent.h"

namespace Engine::Components
{
    class Animator : public Abstract::Component
    {
    public:
        INTERNAL_COMP_CHECK_CONSTEXPR(COM_T_ANIMATOR)

        Animator(const WeakObject& owner);
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;

        void SetModel(const WeakModel & model);
        void SetAnimation(const std::string& animation_name);

    private:
        SERIALIZER_ACCESS
        Animator();

        std::string m_current_animation_;
        StrongModel m_model_;

    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::Animator)
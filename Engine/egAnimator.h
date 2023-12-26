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

        void              SetAnimation(const WeakBaseAnimation & anim);
        WeakBaseAnimation GetAnimation() const;

    private:
        SERIALIZER_ACCESS
        Animator();

        StrongBaseAnimation m_animation_;

    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::Animator)
#pragma once
#include "egComponent.h"

namespace Engine::Components
{
    class Animator final : public Abstract::Component
    {
    public:
        COMPONENT_T(COM_T_ANIMATOR)

        Animator(const WeakObject& owner);
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;

        void        SetAnimation(const std::string& name);
        std::string GetAnimation() const;
        float       GetFrame() const;

    private:
        SERIALIZER_ACCESS
        Animator();

        void UpdateTransform(const StrongTransform & tr, const StrongBaseAnimation & anim) const;

        std::string m_animation_name_;
        float m_current_frame_;

    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::Animator)
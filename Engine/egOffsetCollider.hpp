#pragma once
#include "egBaseCollider.hpp"

namespace Engine::Components
{
    class OffsetCollider : public BaseCollider
    {
    public:
        OffsetCollider(const WeakObject& owner);
        ~OffsetCollider() override;
        void   FixedUpdate(const float& dt) override;
        void   Initialize() override;
        void   OnDeserialized() override;
        void   OnImGui() override;
        void   PostUpdate(const float& dt) override;
        void   PreUpdate(const float& dt) override;
        void   Update(const float& dt) override;

        void SetTransition(const Vector3& transition);
        void SetRotation(const Quaternion& rotation);
        void SetScale(const Vector3& scale);

        Matrix GetLocalMatrix() const override;

    private:
        SERIALIZER_ACCESS
        OffsetCollider();

        Matrix m_transition_;
        Matrix m_rotation_;
        Matrix m_scale_;
    };
}
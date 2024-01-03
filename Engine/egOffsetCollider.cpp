#include "pch.h"
#include "egOffsetCollider.hpp"

SERIALIZER_ACCESS_IMPL(
                       Engine::Components::OffsetCollider,
                       _ARTAG(_BSTSUPER(BaseCollider))
                       _ARTAG(m_transition_)
                       _ARTAG(m_rotation_)
                       _ARTAG(m_scale_))

namespace Engine::Components
{
    OffsetCollider::OffsetCollider(const WeakObject& owner) : BaseCollider(owner) {}

    OffsetCollider::~OffsetCollider() {}

    void OffsetCollider::FixedUpdate(const float& dt)
    {
        BaseCollider::FixedUpdate(dt);
    }

    void OffsetCollider::Initialize()
    {
        BaseCollider::Initialize();
    }

    void OffsetCollider::OnDeserialized()
    {
        BaseCollider::OnDeserialized();
    }

    void OffsetCollider::OnImGui()
    {
        BaseCollider::OnImGui();
    }

    void OffsetCollider::PostUpdate(const float& dt)
    {
        BaseCollider::PostUpdate(dt);
    }

    void OffsetCollider::PreUpdate(const float& dt)
    {
        BaseCollider::PreUpdate(dt);
    }

    void OffsetCollider::Update(const float& dt)
    {
        BaseCollider::Update(dt);
    }

    void OffsetCollider::SetTransition(const Vector3& transition)
    {
        m_transition_ = Matrix::CreateTranslation(transition);
    }

    void OffsetCollider::SetRotation(const Quaternion& rotation)
    {
        m_rotation_ = Matrix::CreateFromQuaternion(rotation);
    }

    void OffsetCollider::SetScale(const Vector3& scale)
    {
        m_scale_ = Matrix::CreateScale(scale);
    }

    Matrix OffsetCollider::GetLocalMatrix() const
    {
        return m_scale_ * m_rotation_ * m_transition_ * BaseCollider::GetLocalMatrix();
    }

    OffsetCollider::OffsetCollider() {}
}

#include "pch.h"
#include "egMouseManager.h"
#include "egManagerHelper.hpp"

namespace Engine::Manager
{
    void MouseManager::Initialize()
    {
        m_current_mouse_position_  = GetNormalizedMousePosition();
        m_previous_mouse_position_ = m_current_mouse_position_;
    }

    void MouseManager::PreUpdate(const float& dt)
    {
        m_current_mouse_position_ = GetNormalizedMousePosition();
        Vector2 delta;
        (m_current_mouse_position_ - m_previous_mouse_position_).Normalize(delta);

        const auto lookRotation =
                Quaternion::CreateFromYawPitchRoll(delta.x * dt, delta.y * dt, 0.f);
        m_mouse_rotation_        = Quaternion::Concatenate(m_mouse_rotation_, lookRotation);
        m_mouse_rotation_matrix_ = Matrix::CreateFromQuaternion(m_mouse_rotation_);
    }

    void MouseManager::Update(const float& dt) {}

    void MouseManager::FixedUpdate(const float& dt) {}

    void MouseManager::PostUpdate(const float& dt)
    {
        m_previous_mouse_position_ = m_current_mouse_position_;
    }

    void MouseManager::PreRender(const float& dt) {}

    void MouseManager::Render(const float& dt) {}

    void MouseManager::PostRender(const float& dt) {}

    Vector2 MouseManager::GetNormalizedMousePosition()
    {
        const Vector2 actual_mouse_position{
            static_cast<float>(GetApplication().GetMouseState().x),
            static_cast<float>(GetApplication().GetMouseState().y)
        };

        const float x = (((2.0f * actual_mouse_position.x) / g_window_width) - 1);
        const float y = -(((2.0f * actual_mouse_position.y) / g_window_height) - 1);

        return {x, y};
    }

    const Quaternion& MouseManager::GetMouseRotation() const
    {
        return m_mouse_rotation_;
    }

    const Matrix& MouseManager::GetMouseRotationMatrix() const
    {
        return m_mouse_rotation_matrix_;
    }
} // namespace Engine::Manager

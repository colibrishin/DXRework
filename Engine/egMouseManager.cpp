#include "pch.h"
#include "egMouseManager.h"

#include "egGlobal.h"
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

    // pitch
    m_mouse_rot_x_ = m_mouse_rot_x_ * Quaternion::CreateFromAxisAngle(Vector3::Up, delta.x * dt);
    // yaw
    m_mouse_rot_y_ = m_mouse_rot_y_ * Quaternion::CreateFromAxisAngle(Vector3::Right, delta.y * dt);
  }

  void MouseManager::Update(const float& dt) {}

  void MouseManager::FixedUpdate(const float& dt) {}

  void MouseManager::PostUpdate(const float& dt) { m_previous_mouse_position_ = m_current_mouse_position_; }

  void MouseManager::PreRender(const float& dt) {}

  void MouseManager::Render(const float& dt) {}

  void MouseManager::PostRender(const float& dt) {}

  Vector2 MouseManager::GetNormalizedMousePosition()
  {
    const Vector2 actual_mouse_position{
      static_cast<float>(GetApplication().GetMouseState().x),
      static_cast<float>(GetApplication().GetMouseState().y)
    };

    const float x = (((2.0f * actual_mouse_position.x) / static_cast<float>(g_window_width)) - 1);
    const float y = -(((2.0f * actual_mouse_position.y) / static_cast<float>(g_window_height)) - 1);

    return {x, y};
  }

  Quaternion MouseManager::GetMouseRotation() const { return m_mouse_rot_x_ * m_mouse_rot_y_; }

  const Quaternion& MouseManager::GetMouseXRotation() const { return m_mouse_rot_x_; }

  const Quaternion& MouseManager::GetMouseYRotation() const { return m_mouse_rot_y_; }
} // namespace Engine::Manager

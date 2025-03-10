#include "InputManager.h"
#include "IInputAPI.h"

namespace Engine::Managers
{
	void InputManager::Initialize()
	{
	}

	void InputManager::PreUpdate(const float dt)
	{
		m_current_mouse_position_ = GetNormalizedMousePosition();
		Vector2 delta;
		(m_current_mouse_position_ - m_previous_mouse_position_).Normalize(delta);

		// pitch
		m_mouse_rot_x_ = m_mouse_rot_x_ * Quaternion::CreateFromAxisAngle(Vector3::Up, delta.x * dt);
		// yaw
		m_mouse_rot_y_ = m_mouse_rot_y_ * Quaternion::CreateFromAxisAngle(Vector3::Down, delta.y * dt);
	}

	void InputManager::Update(const float dt) {}

	void InputManager::FixedUpdate(const float dt) {}

	void InputManager::PostUpdate(const float dt)
	{
		m_previous_mouse_position_ = m_current_mouse_position_;
	}

	bool InputManager::HasScrollChanged(int& value) const
	{
		IInputAPI& ii = g_input_accessor.GetInterface();

		if (ii.HasScrollWheelChanged())
		{
			if (ii.GetPreviousScrollWheelValue() < ii.GetScrollWheelValue())
			{
				value = 1;
			}
			else
			{
				value = -1;
			}
			return true;
		}

		value = 0;
		return false;
	}

	void InputManager::PreRender(const float dt) {}

	void InputManager::Render(const float dt) {}

	void InputManager::PostRender(const float dt) {}

	Vector2 InputManager::GetNormalizedMousePosition()
	{
		IInputAPI& ii = g_input_accessor.GetInterface();

		const Vector2 actual_mouse_position{
			static_cast<float>(ii.GetMouseX()),
			static_cast<float>(ii.GetMouseY())
		};

		const float x = (((2.0f * actual_mouse_position.x) / static_cast<float>(CFG_WIDTH)) - 1);
		const float y = -(((2.0f * actual_mouse_position.y) / static_cast<float>(CFG_HEIGHT)) - 1);

		return { x, y };
	}

	Quaternion InputManager::GetMouseRotation() const
	{
		return m_mouse_rot_x_ * m_mouse_rot_y_;
	}

	const Quaternion& InputManager::GetMouseXRotation() const
	{
		return m_mouse_rot_x_;
	}

	const Quaternion& InputManager::GetMouseYRotation() const
	{
		return m_mouse_rot_y_;
	}
} // namespace Engine::Manager

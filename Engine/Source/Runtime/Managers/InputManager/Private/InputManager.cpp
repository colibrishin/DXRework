#include "../Public/InputManager.h"
#include "Source/Runtime/Managers/WinAPIWrapper/Public/WinAPIWrapper.hpp"

namespace Engine::Managers
{
	void InputManager::Initialize()
	{
		m_current_mouse_position_  = GetNormalizedMousePosition();
		m_previous_mouse_position_ = m_current_mouse_position_;

		m_mouse_ = std::make_unique<DirectX::Mouse>();
		m_keyboard_ = std::make_unique<DirectX::Keyboard>();

		m_mouse_->SetWindow(WinAPI::WinAPIWrapper::GetHWND());

		WinAPI::WinAPIWrapper::RegisterHandler(std::bind_front(&InputManager::MessageHandler, this));
	}

	void InputManager::PreUpdate(const float& dt)
	{
		m_current_mouse_position_ = GetNormalizedMousePosition();
		Vector2 delta;
		(m_current_mouse_position_ - m_previous_mouse_position_).Normalize(delta);

		// pitch
		m_mouse_rot_x_ = m_mouse_rot_x_ * Quaternion::CreateFromAxisAngle(Vector3::Up, delta.x * dt);
		// yaw
		m_mouse_rot_y_ = m_mouse_rot_y_ * Quaternion::CreateFromAxisAngle(Vector3::Right, delta.y * dt);
	}

	void InputManager::Update(const float& dt) {}

	void InputManager::FixedUpdate(const float& dt) {}

	void InputManager::PostUpdate(const float& dt)
	{
		m_previous_mouse_position_ = m_current_mouse_position_;
	}

	bool InputManager::HasKeyChanged(const DirectX::Keyboard::Keys key) const
	{
		return m_previous_keyboard_state_.IsKeyUp(key) && m_keyboard_->GetState().IsKeyDown(key);
	}

	bool InputManager::IsKeyPressed(const DirectX::Keyboard::Keys key) const
	{
		return m_previous_keyboard_state_.IsKeyDown(key) && m_keyboard_->GetState().IsKeyDown(key);
	}

	bool InputManager::HasScrollChanged(int& value) const
	{
		if (m_previous_mouse_state_.scrollWheelValue != m_mouse_->GetState().scrollWheelValue)
		{
			if (m_previous_mouse_state_.scrollWheelValue < m_mouse_->GetState().scrollWheelValue)
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

	DirectX::Mouse::State InputManager::GetMouseState() const
	{
		return m_mouse_->GetState();
	}

	DirectX::Keyboard::State InputManager::GetKeyboardState() const
	{
		return m_keyboard_->GetState();
	}

	void InputManager::PreRender(const float& dt) {}

	void InputManager::Render(const float& dt) {}

	void InputManager::PostRender(const float& dt) {}

	Vector2 InputManager::GetNormalizedMousePosition()
	{
		const DirectX::Mouse::State state = Managers::InputManager::GetInstance().GetMouseState();

		const Vector2 actual_mouse_position{
			static_cast<float>(state.x),
			static_cast<float>(state.y)
		};

		const float x = (((2.0f * actual_mouse_position.x) / static_cast<float>(CFG_WIDTH)) - 1);
		const float y = -(((2.0f * actual_mouse_position.y) / static_cast<float>(CFG_HEIGHT)) - 1);

		return {x, y};
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

	LRESULT InputManager::MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch (msg)
		{
		case WM_ACTIVATE:
		case WM_ACTIVATEAPP:
			DirectX::Mouse::ProcessMessage(msg, wparam, lparam);
			DirectX::Keyboard::ProcessMessage(msg, wparam, lparam);
			break;
		case WM_INPUT:
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEWHEEL:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_MOUSEHOVER:
			DirectX::Mouse::ProcessMessage(msg, wparam, lparam);
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
			DirectX::Keyboard::ProcessMessage(msg, wparam, lparam);
			break;
		default:
			break;
		}

		return 0;
	}
} // namespace Engine::Manager

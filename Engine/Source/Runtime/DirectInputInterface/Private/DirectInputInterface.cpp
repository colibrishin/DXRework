#include "DirectInputInterface.h"
#include "DirectInputInterfaceModule.h"

#include <bit>
#include "WinAPIWrapper.hpp"

void Engine::DirectInputInterface::Initialize()
{
    m_mouse_ = std::make_unique<DirectX::Mouse>();
    m_keyboard_ = std::make_unique<DirectX::Keyboard>();

    m_mouse_->SetWindow(WinAPI::WinAPIWrapper::GetHWND());

    WinAPI::WinAPIWrapper::RegisterHandler("DirectInputInterface", std::bind_front(&DirectInputInterface::MessageHandler, this));
}

void Engine::DirectInputInterface::Shutdown()
{
    WinAPI::WinAPIWrapper::UnregisterHandler("DirectInputInterface");
}

void Engine::DirectInputInterface::Update()
{
	m_previous_keyboard_state_ = m_current_keyboard_state_;
	m_previous_mouse_state_ = m_current_mouse_state_;

	m_current_keyboard_state_ = std::bit_cast<decltype(m_current_keyboard_state_)>(m_keyboard_->GetState());
	m_current_mouse_state_ = std::bit_cast<decltype(m_current_mouse_state_)>(m_mouse_->GetState());
}

LRESULT Engine::DirectInputInterface::MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
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

bool Engine::DirectInputInterface::IsKeyDown(const Keys key) const noexcept
{
    return m_current_keyboard_state_.IsKeyDown(key);
}

bool Engine::DirectInputInterface::IsKeyPressed(const Keys key) const noexcept
{
	return m_previous_keyboard_state_.IsKeyUp(key) && m_current_keyboard_state_.IsKeyDown(key);
}

bool Engine::DirectInputInterface::IsKeyReleased(const Keys key) const noexcept
{
	return m_previous_keyboard_state_.IsKeyDown(key) && m_current_keyboard_state_.IsKeyUp(key);
}

bool Engine::DirectInputInterface::IsKeyDown(const eMouseButtonEnum key) const noexcept
{
	switch (key)
	{
	case MOUSE_LEFT:
		return m_current_mouse_state_.leftButton;
	case MOUSE_MIDDLE:
		return m_current_mouse_state_.middleButton;
	case MOUSE_RIGHT:
		return m_current_mouse_state_.rightButton;
	case MOUSE_4:
		return m_current_mouse_state_.xButton1;
	case MOUSE_5:
		return m_current_mouse_state_.xButton2;
	default:
		return false;
	}
}

bool Engine::DirectInputInterface::IsKeyPressed(const eMouseButtonEnum key) const noexcept
{
	switch (key)
	{
	case MOUSE_LEFT:
		return !m_previous_mouse_state_.leftButton && m_current_mouse_state_.leftButton;
	case MOUSE_MIDDLE:
		return !m_previous_mouse_state_.middleButton && m_current_mouse_state_.middleButton;
	case MOUSE_RIGHT:
		return !m_previous_mouse_state_.rightButton && m_current_mouse_state_.rightButton;
	case MOUSE_4:
		return !m_previous_mouse_state_.xButton1 && m_current_mouse_state_.xButton1;
	case MOUSE_5:
		return !m_previous_mouse_state_.xButton2 && m_current_mouse_state_.xButton2;
	default:
		return false;
	}
}

bool Engine::DirectInputInterface::IsKeyReleased(const eMouseButtonEnum key) const noexcept
{
	switch (key)
	{
	case MOUSE_LEFT:
		return m_previous_mouse_state_.leftButton && !m_current_mouse_state_.leftButton;
	case MOUSE_MIDDLE:
		return m_previous_mouse_state_.middleButton && !m_current_mouse_state_.middleButton;
	case MOUSE_RIGHT:
		return m_previous_mouse_state_.rightButton && !m_current_mouse_state_.rightButton;
	case MOUSE_4:
		return m_previous_mouse_state_.xButton1 && !m_current_mouse_state_.xButton1;
	case MOUSE_5:
		return m_previous_mouse_state_.xButton2 && !m_current_mouse_state_.xButton2;
	default:
		return false;
	}
}

bool Engine::DirectInputInterface::HasScrollWheelChanged() const noexcept
{
	return m_current_mouse_state_.scrollWheelValue != m_previous_mouse_state_.scrollWheelValue;
}

int Engine::DirectInputInterface::GetScrollWheelValue() const noexcept
{
	return m_current_mouse_state_.scrollWheelValue;
}

int Engine::DirectInputInterface::GetPreviousScrollWheelValue() const noexcept
{
	return m_previous_mouse_state_.scrollWheelValue;
}

int Engine::DirectInputInterface::GetMouseX() const noexcept
{
	return m_current_mouse_state_.x;
}

int Engine::DirectInputInterface::GetMouseY() const noexcept
{
	return m_current_mouse_state_.y;
}

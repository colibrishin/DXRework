#pragma once
#include "Windows.h"
#define WIN32_LEAN_AND_MEAN

#include <directxtk12/Mouse.h>
#include <directxtk12/Keyboard.h>
#include <bit>

#include "InputInterface.h"
#include "DirectInputInterface.generated.h"

namespace Engine 
{
	ECLASS()
	struct ENGINE_DIRECTINPUTINTERFACE_API DirectInputInterface : public InputInterface
	{
		// InputInterface을(를) 통해 상속됨
		void Initialize() override;
		void Shutdown() override;
		void Update() override;
		LRESULT MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override;
		
		bool IsKeyDown(const Keys key) const noexcept override;
		bool IsKeyPressed(const Keys key) const noexcept override;
		bool IsKeyReleased(const Keys key) const noexcept override;

		bool IsKeyDown(const eMouseButtonEnum key) const noexcept override;
		bool IsKeyPressed(const eMouseButtonEnum key) const noexcept override;
		bool IsKeyReleased(const eMouseButtonEnum key) const noexcept override;
		
		bool HasScrollWheelChanged() const noexcept override;
		int  GetScrollWheelValue() const noexcept override;
		int  GetPreviousScrollWheelValue() const noexcept override;

		int GetMouseX() const noexcept override;
		int GetMouseY() const noexcept override;

	private:
		Unique<DirectX::Keyboard> m_keyboard_ = nullptr;
		Unique<DirectX::Mouse> m_mouse_ = nullptr;

		MouseState m_previous_mouse_state_;
		KeyboardState m_previous_keyboard_state_;

		MouseState m_current_mouse_state_;
		KeyboardState m_current_keyboard_state_;
	};
}
#pragma once
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"

#if PLATFORM == Windows
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#if USE_DX12
#include <directxtk12/Mouse.h>
#include <directxtk12/Keyboard.h>
#endif

#include "TypeLibrary/Public/TypeLibrary.h"

#include "InputManager.generated.h"

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_INPUTMANAGER_API InputManager : public Abstracts::Singleton<InputManager>
	{
		GENERATE_BODY
	public:
		InputManager(SINGLETON_LOCK_TOKEN)
			: Singleton<InputManager>() {};
		void Initialize() override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		bool HasKeyChanged(const DirectX::Keyboard::Keys key) const;
		bool IsKeyPressed(const DirectX::Keyboard::Keys key) const;
		bool HasScrollChanged(int& value) const;

		DirectX::Mouse::State GetMouseState() const;
		DirectX::Keyboard::State GetKeyboardState() const;

		static Vector2 GetNormalizedMousePosition();

		[[nodiscard]] Quaternion        GetMouseRotation() const;
		[[nodiscard]] const Quaternion& GetMouseXRotation() const;
		[[nodiscard]] const Quaternion& GetMouseYRotation() const;

	private:
		friend struct SingletonDeleter;
		~InputManager() override = default;

		LRESULT MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		Quaternion m_mouse_rot_x_;
		Quaternion m_mouse_rot_y_;

		std::unique_ptr<DirectX::Mouse> m_mouse_;
		std::unique_ptr<DirectX::Keyboard> m_keyboard_;

		DirectX::Mouse::State m_previous_mouse_state_;
		DirectX::Keyboard::State m_previous_keyboard_state_;

		Vector2 m_previous_mouse_position_;
		Vector2 m_current_mouse_position_;
	};
} // namespace Engine::Managers

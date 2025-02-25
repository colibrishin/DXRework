#pragma once
#include "Singleton.h"

#if PLATFORM == Windows
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#if USE_DX12
#include <directxtk12/Mouse.h>
#include <directxtk12/Keyboard.h>
#endif

#include "TypeLibrary.h"
#include "InputInterface.h"

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

		template <typename Enum>
		bool IsKeyDown(const Enum key) const
		{
			InputInterface& ii = InputInterfaceAccessor::GetInterface();
			return ii.IsKeyDown(key);
		}

		template <typename Enum>
		bool IsKeyPressed(const Enum key) const
		{
			InputInterface& ii = InputInterfaceAccessor::GetInterface();
			return ii.IsKeyPressed(key);
		}

		template <typename Enum>
		bool IsKeyReleased(const Enum key) const
		{
			InputInterface& ii = InputInterfaceAccessor::GetInterface();
			return ii.IsKeyReleased(key);
		}

		bool HasScrollChanged(int& value) const;

		static Vector2 GetNormalizedMousePosition();

		[[nodiscard]] Quaternion        GetMouseRotation() const;
		[[nodiscard]] const Quaternion& GetMouseXRotation() const;
		[[nodiscard]] const Quaternion& GetMouseYRotation() const;

	private:
		friend struct SingletonDeleter;
		~InputManager() override = default;

		Quaternion m_mouse_rot_x_;
		Quaternion m_mouse_rot_y_;

		Vector2 m_previous_mouse_position_;
		Vector2 m_current_mouse_position_;
	};
} // namespace Engine::Managers

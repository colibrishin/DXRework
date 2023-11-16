#pragma once
#include <memory>

#include <Keyboard.h>
#include <Mouse.h>

#include "egManager.hpp"
#include "StepTimer.hpp"

namespace Engine::Manager
{
	using namespace DirectX;

	class Application final : public Abstract::Singleton<Application, HWND>
	{
	public:
		Application(SINGLETON_LOCK_TOKEN) : Singleton() { }
		~Application() override = default;

		void Initialize(HWND hwnd) override;
		static void UpdateWindowSize(HWND hWnd);
		void Tick();

		LRESULT MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		float GetDeltaTime() const { return static_cast<float>(m_timer->GetElapsedSeconds()); }
		uint32_t GetFPS() const { return m_timer->GetFramesPerSecond(); }

		Keyboard::State GetKeyState() const { return m_keyboard->GetState(); }
		Mouse::State GetMouseState() const { return m_mouse->GetState(); }

	private:
		void PreUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;

	private:
		HWND m_hWnd = nullptr;

		// Input
		std::unique_ptr<Keyboard> m_keyboard;
		std::unique_ptr<Mouse> m_mouse;

		// Time
		std::unique_ptr<DX::StepTimer> m_timer;
	};
}

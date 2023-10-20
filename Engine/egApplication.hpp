#pragma once
#include <memory>
#include "Keyboard.h"
#include "Mouse.h"
#include "StepTimer.hpp"

namespace Engine
{
	inline std::atomic<bool> g_full_screen = false;
	inline std::atomic<bool> g_vsync_enabled = true;
	inline std::atomic<UINT> g_window_width = 800;
	inline std::atomic<UINT> g_window_height = 600;

	inline std::atomic<float> g_screen_near = 0.1f;
	inline std::atomic<float> g_screen_far = 1000.0f;

	using namespace DirectX;

	class Application final
	{
	public:
		~Application() = default;
		Application(const Application& other) = delete;

		static void UpdateWindowSize(HWND hWnd);
		static void Initialize(HWND hWnd);
		static void Tick();

		static void PreUpdate();
		static void Update();
		static void PreRender();
		static void Render();

		static LRESULT MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
		static HWND GetWindowHandle() { return s_WindowHandle; }
		static float GetDeltaTime() { return static_cast<float>(s_timer->GetElapsedSeconds()); }

	private:
		Application() = default;

		static std::unique_ptr<Application> s_Instance;
		inline static HWND s_WindowHandle = nullptr;

		inline static std::unique_ptr<Keyboard> s_keyboard = nullptr;
		inline static std::unique_ptr<Mouse> s_mouse = nullptr;
		inline static std::unique_ptr<DX::StepTimer> s_timer = nullptr;
	};
}

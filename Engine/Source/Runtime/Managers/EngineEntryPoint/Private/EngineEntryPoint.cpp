#include "../Public/EngineEntryPoint.h"

#if PLATFORM == WINDOWS
#include "Source/Runtime/Managers/WinAPIWrapper/Public/WinAPIWrapper.hpp"
#endif

/*
#include "imgui.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND   hWnd,
	UINT   msg,
	WPARAM wParam,
	LPARAM lParam
);
*/

bool Engine::Managers::EngineEntryPoint::s_instantiated_ = false;
std::atomic<bool> Engine::Managers::EngineEntryPoint::s_paused = false;
std::atomic<float> Engine::Managers::EngineEntryPoint::s_fixed_update_interval = 1 / 30.f;

namespace Engine::Managers
{
	void EngineEntryPoint::UpdateWindowSize()
	{
#if PLATFORM == WINDOWS
		HWND hWnd = WinAPI::WinAPIWrapper::GetHWND();

		SetWindowPos
				(
				 hWnd, nullptr,
				 (GetSystemMetrics(SM_CXSCREEN) - CFG_WIDTH) / 2,
				 (GetSystemMetrics(SM_CYSCREEN) - CFG_HEIGHT) / 2,
				 CFG_WIDTH, CFG_HEIGHT, SWP_NOMOVE | SWP_NOZORDER
				);

		ShowWindow(hWnd, SW_SHOW);
		SetForegroundWindow(hWnd);
		SetFocus(hWnd);
#endif
	}

	EngineEntryPoint::EngineEntryPoint(SINGLETON_LOCK_TOKEN)
		: Singleton()
	{
		if (s_instantiated_)
		{
			throw std::runtime_error("EngineEntryPoint is already instantiated");
		}

		s_instantiated_ = true;
		std::set_terminate(SIGTERM);
	}

	float EngineEntryPoint::GetDeltaTime() const
	{
		return static_cast<float>(m_timer->GetElapsedSeconds());
	}

	uint32_t EngineEntryPoint::GetFPS() const
	{
		return m_timer->GetFramesPerSecond();
	}

	uint32_t EngineEntryPoint::GetWidth() const
	{
		return CFG_WIDTH;
	}

	uint32_t EngineEntryPoint::GetHeight() const
	{
		return CFG_HEIGHT;
	}

	EngineEntryPoint::~EngineEntryPoint()
	{
		SIGTERM();
	}

	void EngineEntryPoint::Initialize()
	{
		UpdateWindowSize();
	}

	void EngineEntryPoint::Tick()
	{
		static auto internal_tick = std::bind_front(&EngineEntryPoint::tickInternal, this);
		m_timer->Tick(internal_tick);
	}

	void EngineEntryPoint::PreUpdate(const float& dt)
	{
	}

	void EngineEntryPoint::FixedUpdate(const float& dt)
	{
	}

	void EngineEntryPoint::Update(const float& dt)
	{
	}

	void EngineEntryPoint::PreRender(const float& dt)
	{
	}

	void EngineEntryPoint::Render(const float& dt)
	{
	}

	void EngineEntryPoint::PostRender(const float& dt)
	{
	}

	void EngineEntryPoint::PostUpdate(const float& dt)
	{
	}

	void EngineEntryPoint::tickInternal()
	{
		static float elapsed = 0.f;

		float dt = GetDeltaTime();

		if (s_paused)
		{
			elapsed = 0.f;
			dt      = 0.f;
		}

		while (elapsed >= s_fixed_update_interval)
		{
			FixedUpdate(s_fixed_update_interval);
			elapsed -= s_fixed_update_interval;
		}
		
		PreUpdate(dt);
		Update(dt);
		PostUpdate(dt);

		PreRender(dt);
		Render(dt);
		PostRender(dt);

		elapsed += dt;
	}

	void EngineEntryPoint::SIGTERM()
	{
	}

	LRESULT EngineEntryPoint::MessageHandler(
		HWND   hwnd, UINT msg, WPARAM wparam,
		LPARAM lparam
	)
	{
		switch (msg)
		{
		case WM_ACTIVATE:
		case WM_ACTIVATEAPP:
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
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
			break;

		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
	}
} // namespace Engine::Manager

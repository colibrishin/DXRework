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
std::atomic<bool> Engine::Managers::EngineEntryPoint::s_fullscreen = false;
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

	bool EngineEntryPoint::IsFullScreen() const
	{
		return s_fullscreen;
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
} // namespace Engine::Manager

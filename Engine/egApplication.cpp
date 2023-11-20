#include "pch.hpp"
#include "egApplication.hpp"

#include "Keyboard.h"
#include "Mouse.h"

#include "egCollisionDetector.hpp"
#include "egD3Device.hpp"
#include "egManagerHelper.hpp"
#include "egProjectionFrustum.hpp"
#include "egSceneManager.hpp"
#include "egToolkitAPI.hpp"

namespace Engine::Manager
{
	void Application::UpdateWindowSize(HWND hWnd)
	{
		SetWindowPos(
			hWnd,
			nullptr,
			(GetSystemMetrics(SM_CXSCREEN) - g_window_width) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - g_window_height) / 2,
			g_window_width,
			g_window_height,
			SWP_NOMOVE | SWP_NOZORDER);

		ShowWindow(hWnd, SW_SHOW);
		SetForegroundWindow(hWnd);
		SetFocus(hWnd);
	}

	void Application::Initialize(HWND hWnd)
	{
		m_keyboard = std::make_unique<Keyboard>();
		m_mouse = std::make_unique<Mouse>();
		m_mouse->SetWindow(hWnd);
		m_timer = std::make_unique<DX::StepTimer>();
		UpdateWindowSize(hWnd);

		GetD3Device().Initialize(hWnd);
		GetToolkitAPI().Initialize();
		GetRenderPipeline().Initialize();

		GetCollisionDetector().Initialize();
		GetProjectionFrustum().Initialize();
		GetResourceManager().Initialize();
		GetSceneManager().Initialize();
	}

	void Application::Tick()
	{
		static float elapsed = 0.0f;

		if (m_keyboard->GetState().Escape)
		{
			PostQuitMessage(0);
		}

		m_timer->Tick([&]()
		{
			const auto dt = static_cast<float>(m_timer->GetElapsedSeconds());
			elapsed += dt;

			PreUpdate(dt);

			if (elapsed >= g_fixed_update_interval)
			{
				FixedUpdate(dt);
				elapsed = 0.0f;
			}

			Update(dt);

			PreRender(dt);
			Render(dt);
		});
	}

	void Application::PreUpdate(const float& dt)
	{
		GetSceneManager().PreUpdate(dt);
		GetProjectionFrustum().PreUpdate(dt);
		GetResourceManager().PreUpdate(dt);
		GetCollisionDetector().PreUpdate(dt);
		GetPhysicsManager().PreUpdate(dt);
		GetConstraintSolver().PreUpdate(dt);
		GetTransformLerpManager().PreUpdate(dt);
		GetD3Device().PreUpdate(dt);
	}

	void Application::FixedUpdate(const float& dt)
	{
		GetSceneManager().FixedUpdate(dt);
		GetProjectionFrustum().FixedUpdate(dt);
		GetResourceManager().FixedUpdate(dt);
		GetCollisionDetector().FixedUpdate(dt);
		GetPhysicsManager().FixedUpdate(dt);
		GetConstraintSolver().FixedUpdate(dt);
		GetTransformLerpManager().FixedUpdate(dt);
		GetD3Device().FixedUpdate(dt);
	}

	void Application::Update(const float& dt)
	{
		GetSceneManager().Update(dt);
		GetProjectionFrustum().Update(dt);
		GetResourceManager().Update(dt);
		GetCollisionDetector().Update(dt);
		GetPhysicsManager().Update(dt);
		GetConstraintSolver().Update(dt);
		GetTransformLerpManager().Update(dt);
		GetD3Device().Update(dt);
	}

	void Application::PreRender(const float& dt)
	{
		GetToolkitAPI().PreRender(dt);
		GetSceneManager().PreRender(dt);
		GetProjectionFrustum().PreRender(dt);
		GetResourceManager().PreRender(dt);
		GetCollisionDetector().PreRender(dt);
		GetPhysicsManager().PreRender(dt);
		GetConstraintSolver().PreRender(dt);
		GetTransformLerpManager().PreRender(dt);
		GetRenderPipeline().PreRender(dt);
		GetD3Device().PreRender(dt);
	}

	void Application::Render(const float& dt)
	{
		GetSceneManager().Render(dt);
		GetProjectionFrustum().Render(dt);
		GetResourceManager().Render(dt);
		GetCollisionDetector().Render(dt);
		GetPhysicsManager().Render(dt);
		GetConstraintSolver().PreRender(dt);
		GetTransformLerpManager().Render(dt);
		GetToolkitAPI().Render(dt);
		GetD3Device().Render(dt);
	}

	LRESULT Application::MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch (msg)
		{
		case WM_ACTIVATE:
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
			Mouse::ProcessMessage(msg, wparam, lparam);
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
			Keyboard::ProcessMessage(msg, wparam, lparam);
			break;

		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
	}
}

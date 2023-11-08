#include "pch.hpp"
#include "egApplication.hpp"

#include "Keyboard.h"
#include "Mouse.h"

#include "egCollisionManager.hpp"
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

		GetCollisionManager().Initialize();
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
			PreUpdate();
			Update();
		});

		elapsed += static_cast<float>(m_timer->GetElapsedSeconds());

		if (elapsed <= 1.0f / 33.0f)
		{
			FixedUpdate();
		}

		PreRender();

		Render();
	}

	void Application::PreUpdate()
	{
		GetCollisionManager().PreUpdate();
		GetSceneManager().PreUpdate();
		GetProjectionFrustum().PreUpdate();
		GetResourceManager().PreUpdate();
		GetD3Device().PreUpdate();
	}

	void Application::FixedUpdate()
	{
		GetCollisionManager().FixedUpdate();
		GetSceneManager().FixedUpdate();
		GetProjectionFrustum().FixedUpdate();
		GetResourceManager().FixedUpdate();
		GetD3Device().FixedUpdate();
	}

	void Application::Update()
	{
		GetCollisionManager().Update();
		GetSceneManager().Update();
		GetProjectionFrustum().Update();
		GetResourceManager().Update();
		GetD3Device().Update();
	}

	void Application::PreRender()
	{
		GetToolkitAPI().PreRender();
		GetCollisionManager().PreRender();
		GetSceneManager().PreRender();
		GetProjectionFrustum().PreRender();
		GetResourceManager().PreRender();
		GetRenderPipeline().PreRender();
		GetD3Device().PreRender();
	}

	void Application::Render()
	{
		GetCollisionManager().Render();
		GetSceneManager().Render();
		GetProjectionFrustum().Render();
		GetResourceManager().Render();
		GetToolkitAPI().Render();
		GetD3Device().Render();
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

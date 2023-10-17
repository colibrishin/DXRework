#include "pch.hpp"
#include "egApplication.hpp"

#include "egD3Device.hpp"
#include "egSceneManager.hpp"

namespace Engine
{
	std::unique_ptr<Application> Application::s_Instance = nullptr;

	void Application::UpdateWindowSize(HWND hWnd)
	{
		SetWindowPos(
			hWnd, 
			0, 
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
		if (!s_Instance)
		{
			s_Instance = std::unique_ptr<Application>(new Application());
		}

		s_WindowHandle = hWnd;

		s_keyboard = std::make_unique<DirectX::Keyboard>();
		s_mouse = std::make_unique<Mouse>();
		s_mouse->SetWindow(hWnd);
		s_timer = std::make_unique<DX::StepTimer>();

		Graphic::D3Device::Initialize(hWnd);
		UpdateWindowSize(hWnd);
	}

	void Application::Tick()
	{
		PreUpdate();

		s_timer->Tick([&]()
		{
			Update();
		});

		PreRender();

		Render();
	}

	void Application::PreUpdate()
	{
		Manager::SceneManager::GetInstance()->PreUpdate();
	}

	void Application::Update()
	{
		Manager::SceneManager::GetInstance()->Update();
	}

	void Application::PreRender()
	{
		Manager::SceneManager::GetInstance()->PreRender();
		Graphic::D3Device::FrameBegin();
	}

	void Application::Render()
	{
		Manager::SceneManager::GetInstance()->Render();
		Graphic::D3Device::Present();
	}

	LRESULT Application::MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch(msg)
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

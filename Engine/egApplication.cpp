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

	Application::~Application()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void Application::Initialize(HWND hWnd)
	{
		m_keyboard = std::make_unique<Keyboard>();
		m_mouse = std::make_unique<Mouse>();
		m_mouse->SetWindow(hWnd);
		m_timer = std::make_unique<DX::StepTimer>();
		UpdateWindowSize(hWnd);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		GetD3Device().Initialize(hWnd);
		GetToolkitAPI().Initialize();
		GetRenderPipeline().Initialize();

		GetCollisionDetector().Initialize();
		GetProjectionFrustum().Initialize();
		GetResourceManager().Initialize();
		GetSceneManager().Initialize();
		GetDebugger().Initialize();
		GetTaskScheduler().Initialize();

		ImGui_ImplWin32_Init(hWnd);
		ImGui_ImplDX11_Init(GetD3Device().GetDevice(), GetD3Device().GetContext());
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

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			ImGui::ShowDemoWindow();

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
		GetTaskScheduler().PreUpdate(dt);
		GetCollisionDetector().PreUpdate(dt);
		GetSceneManager().PreUpdate(dt);
		GetProjectionFrustum().PreUpdate(dt);
		GetResourceManager().PreUpdate(dt);
		GetPhysicsManager().PreUpdate(dt);
		GetConstraintSolver().PreUpdate(dt);
		GetTransformLerpManager().PreUpdate(dt);
		GetDebugger().PreUpdate(dt);
		GetD3Device().PreUpdate(dt);
		GetToolkitAPI().PreUpdate(dt);
	}

	void Application::FixedUpdate(const float& dt)
	{
		GetTaskScheduler().FixedUpdate(dt);
		GetCollisionDetector().FixedUpdate(dt);
		GetSceneManager().FixedUpdate(dt);
		GetProjectionFrustum().FixedUpdate(dt);
		GetResourceManager().FixedUpdate(dt);
		GetPhysicsManager().FixedUpdate(dt);
		GetConstraintSolver().FixedUpdate(dt);
		GetTransformLerpManager().FixedUpdate(dt);
		GetDebugger().FixedUpdate(dt);
		GetD3Device().FixedUpdate(dt);
		GetToolkitAPI().FixedUpdate(dt);
	}

	void Application::Update(const float& dt)
	{
		GetTaskScheduler().Update(dt);
		GetCollisionDetector().Update(dt);
		GetSceneManager().Update(dt);
		GetProjectionFrustum().Update(dt);
		GetResourceManager().Update(dt);
		GetPhysicsManager().Update(dt);
		GetConstraintSolver().Update(dt);
		GetTransformLerpManager().Update(dt);
		GetDebugger().Update(dt);
		GetD3Device().Update(dt);
		GetToolkitAPI().Update(dt);
	}

	void Application::PreRender(const float& dt)
	{
		GetTaskScheduler().PreRender(dt);
		GetCollisionDetector().PreRender(dt);
		GetToolkitAPI().PreRender(dt);
		GetSceneManager().PreRender(dt);
		GetProjectionFrustum().PreRender(dt);
		GetResourceManager().PreRender(dt);
		GetPhysicsManager().PreRender(dt);
		GetConstraintSolver().PreRender(dt);
		GetTransformLerpManager().PreRender(dt);
		GetDebugger().PreRender(dt);
		GetRenderPipeline().PreRender(dt);
		GetD3Device().PreRender(dt);
	}

	void Application::Render(const float& dt)
	{
		GetTaskScheduler().Render(dt);
		GetCollisionDetector().Render(dt);
		GetSceneManager().Render(dt);
		GetProjectionFrustum().Render(dt);
		GetResourceManager().Render(dt);
		GetPhysicsManager().Render(dt);
		GetConstraintSolver().PreRender(dt);
		GetTransformLerpManager().Render(dt);
		GetDebugger().Render(dt);
		GetToolkitAPI().Render(dt);

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

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

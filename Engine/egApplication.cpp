#include "pch.h"
#include "egApplication.h"

#include "egGlobal.h"
#include "egManagerHelper.hpp"
#include "imgui_impl_dx12.h"

namespace Engine::Manager
{
	void Application::UpdateWindowSize(HWND hWnd)
	{
		SetWindowPos
				(
				 hWnd, nullptr,
				 (GetSystemMetrics(SM_CXSCREEN) - g_window_width) / 2,
				 (GetSystemMetrics(SM_CYSCREEN) - g_window_height) / 2,
				 g_window_width, g_window_height, SWP_NOMOVE | SWP_NOZORDER
				);

		ShowWindow(hWnd, SW_SHOW);
		SetForegroundWindow(hWnd);
		SetFocus(hWnd);
	}

	Application::Application(SINGLETON_LOCK_TOKEN)
		: Singleton(),
		  m_previous_keyboard_state_(),
		  m_previous_mouse_state_()
	{
		if (s_instantiated_)
		{
			throw std::runtime_error("Application is already instantiated");
		}

		s_instantiated_ = true;
		std::set_terminate(SIGTERM);
	}

	float Application::GetDeltaTime() const
	{
		return static_cast<float>(m_timer->GetElapsedSeconds());
	}

	uint32_t Application::GetFPS() const
	{
		return m_timer->GetFramesPerSecond();
	}

	Keyboard::State Application::GetCurrentKeyState() const
	{
		return m_keyboard->GetState();
	}

	bool Application::HasKeyChanged(const Keyboard::Keys key) const
	{
		return m_previous_keyboard_state_.IsKeyUp(key) && m_keyboard->GetState().IsKeyDown(key);
	}

	bool Application::IsKeyPressed(const Keyboard::Keys key) const
	{
		return m_previous_keyboard_state_.IsKeyDown(key) && m_keyboard->GetState().IsKeyDown(key);
	}

	bool Application::HasScrollChanged(int& value) const
	{
		if (m_previous_mouse_state_.scrollWheelValue != m_mouse->GetState().scrollWheelValue)
		{
			if (m_previous_mouse_state_.scrollWheelValue < m_mouse->GetState().scrollWheelValue)
			{
				value = 1;
			}
			else
			{
				value = -1;
			}
			return true;
		}
		value = 0;
		return false;
	}

	Mouse::State Application::GetMouseState() const
	{
		return m_mouse->GetState();
	}

	Application::~Application()
	{
		SIGTERM();
	}

	void Application::Initialize(HWND hWnd)
	{
		m_keyboard = std::make_unique<Keyboard>();
		m_mouse    = std::make_unique<Mouse>();
		m_mouse->SetWindow(hWnd);
		m_timer = std::make_unique<DX::StepTimer>();
		UpdateWindowSize(hWnd);

		GetResourceManager().Initialize();
		GetSceneManager().Initialize();
		GetTaskScheduler().Initialize();
		GetMouseManager().Initialize();
		GetProjectionFrustum().Initialize();
		GetCollisionDetector().Initialize();
		GetLerpManager().Initialize();
		GetPhysicsManager().Initialize();
		GetConstraintSolver().Initialize();
		GetGraviton().Initialize();

		GetD3Device().Initialize(hWnd);
		GetRenderPipeline().Initialize();
		GetRaytracingPipeline().Initialize();
		GetToolkitAPI().Initialize();
		GetShadowManager().Initialize();
		GetReflectionEvaluator().Initialize();
		GetImGuiManager().Initialize(hWnd);
		GetDebugger().Initialize();
		GetRenderer().Initialize();
		GetRayTracer().Initialize();
	}

	void Application::Tick()
	{
		m_timer->Tick
				(
				 [&]()
				 {
					 tickInternal();
				 }
				);
	}

	void Application::PreUpdate(const float& dt)
	{
		GetToolkitAPI().PreUpdate(dt);
		GetTaskScheduler().PreUpdate(dt);
		GetMouseManager().PreUpdate(dt);
		GetCollisionDetector().PreUpdate(dt);
		GetReflectionEvaluator().PreUpdate(dt);
		GetSceneManager().PreUpdate(dt);
		GetShadowManager().PreUpdate(dt);
		GetResourceManager().PreUpdate(dt);
		GetGraviton().PreUpdate(dt);
		GetConstraintSolver().PreUpdate(dt);
		GetPhysicsManager().PreUpdate(dt);
		GetLerpManager().PreUpdate(dt);
		GetProjectionFrustum().PreUpdate(dt);

		GetRenderer().PreUpdate(dt);
		GetShadowManager().PreUpdate(dt);
		GetDebugger().PreUpdate(dt);
		GetD3Device().PreUpdate(dt);
		GetRenderPipeline().PreUpdate(dt);
	}

	void Application::FixedUpdate(const float& dt)
	{
		GetTaskScheduler().FixedUpdate(dt);
		// collider or world update
		GetSceneManager().FixedUpdate(dt);

		// physics updates.
		// gravity
		GetGraviton().FixedUpdate(dt);
		// collision detection
		GetCollisionDetector().FixedUpdate(dt);
		// constraint solver
		GetConstraintSolver().FixedUpdate(dt);
		// apply forces
		GetPhysicsManager().FixedUpdate(dt);
		// lerp rigidbody movements
		GetLerpManager().FixedUpdate(dt);

		GetMouseManager().FixedUpdate(dt);
		GetReflectionEvaluator().FixedUpdate(dt);
		GetShadowManager().FixedUpdate(dt);
		GetResourceManager().FixedUpdate(dt);

		GetProjectionFrustum().FixedUpdate(dt);
		GetRenderer().FixedUpdate(dt);
		GetShadowManager().FixedUpdate(dt);
		GetDebugger().FixedUpdate(dt);
		GetD3Device().FixedUpdate(dt);
		GetToolkitAPI().FixedUpdate(dt);
	}

	void Application::Update(const float& dt)
	{
		GetTaskScheduler().Update(dt);
		GetMouseManager().Update(dt);
		GetCollisionDetector().Update(dt);
		GetReflectionEvaluator().Update(dt);
		GetSceneManager().Update(dt);
		GetResourceManager().Update(dt);
		GetGraviton().Update(dt);
		GetConstraintSolver().Update(dt);
		GetPhysicsManager().Update(dt);
		GetLerpManager().Update(dt);
		GetProjectionFrustum().Update(dt);

		GetRenderer().Update(dt);
		GetShadowManager().Update(dt); // update light information
		GetDebugger().Update(dt); // update debug flag
		GetD3Device().Update(dt);
		GetToolkitAPI().Update(dt); //fmod update
	}

	void Application::PreRender(const float& dt)
	{
		GetTaskScheduler().PreRender(dt);
		GetMouseManager().PreRender(dt);
		GetCollisionDetector().PreRender(dt);
		GetToolkitAPI().PreRender(dt);
		GetReflectionEvaluator().PreRender(dt);
		GetSceneManager().PreRender(dt);
		GetResourceManager().PreRender(dt);
		GetGraviton().PreRender(dt);
		GetConstraintSolver().PreRender(dt);
		GetPhysicsManager().PreRender(dt);
		GetLerpManager().PreRender(dt);
		GetProjectionFrustum().PreRender(dt);
		GetDebugger().PreRender(dt);
		GetD3Device().PreRender(dt);

		if (g_raytracing)
		{
			GetRayTracer().PreRender(dt); // pre-process render information
			GetRaytracingPipeline().PreRender(dt); // clean up rtv, dsv, etc.
		}
		else
		{
			GetRenderer().PreRender(dt); // pre-process render information
			GetShadowManager().PreRender(dt); // shadow resource command, executing shadow pass, set shadow resources.
			GetRenderPipeline().PreRender(dt); // clean up rtv, dsv, etc.
		}
	}

	void Application::Render(const float& dt)
	{
		GetTaskScheduler().Render(dt);
		GetMouseManager().Render(dt);
		GetCollisionDetector().Render(dt);
		GetReflectionEvaluator().Render(dt);
		GetSceneManager().Render(dt);
		GetResourceManager().Render(dt);
		GetGraviton().Render(dt);
		GetConstraintSolver().PreRender(dt);
		GetPhysicsManager().Render(dt);
		GetLerpManager().Render(dt);
		GetProjectionFrustum().Render(dt);

		if (g_raytracing)
		{
			GetRayTracer().Render(dt);

			GetRaytracingPipeline().Render(dt);
		}
		else
		{
			// Shadow resource binding
			GetShadowManager().Render(dt);

			// Render commands (opaque)
			GetRenderer().Render(dt);
			GetRenderPipeline().Render(dt);
		}

		GetImGuiManager().Render(dt);
		GetDebugger().Render(dt);
		GetToolkitAPI().Render(dt);
		GetD3Device().Render(dt);
	}

	void Application::PostRender(const float& dt)
	{
		GetTaskScheduler().PostRender(dt);
		GetMouseManager().PostRender(dt);
		GetCollisionDetector().PostRender(dt);
		GetSceneManager().PostRender(dt);
		GetResourceManager().PostRender(dt);
		GetGraviton().PostRender(dt);
		GetConstraintSolver().PostRender(dt);
		GetPhysicsManager().PostRender(dt);
		GetLerpManager().PostRender(dt);
		GetProjectionFrustum().PostRender(dt);
		GetDebugger().PostRender(dt); // gather information until render

		GetRenderer().PostRender(dt);
		GetToolkitAPI().PostRender(dt); // toolkit related render commands
		GetShadowManager().PostRender(dt);

		GetImGuiManager().PostRender(dt);

		GetReflectionEvaluator().PostRender(dt);
		GetRenderPipeline().PostRender(dt); // Wrap up command lists, present
		GetD3Device().PostRender(dt);
	}

	void Application::PostUpdate(const float& dt)
	{
		GetTaskScheduler().PostUpdate(dt);
		GetMouseManager().PostUpdate(dt);
		GetCollisionDetector().PostUpdate(dt);
		GetSceneManager().PostUpdate(dt);
		GetResourceManager().PostUpdate(dt);
		GetGraviton().PostUpdate(dt);
		GetConstraintSolver().PostUpdate(dt);
		GetPhysicsManager().PostUpdate(dt);
		GetLerpManager().PostUpdate(dt);
		GetProjectionFrustum().PostUpdate(dt);
		GetRenderer().PostUpdate(dt);
		GetShadowManager().PostUpdate(dt);
		GetDebugger().PostUpdate(dt);
		GetD3Device().PostUpdate(dt);
		GetToolkitAPI().PostUpdate(dt);
		GetReflectionEvaluator().PostUpdate(dt);
		GetRenderPipeline().PostUpdate(dt);
	}

	void Application::tickInternal()
	{
		static float elapsed = 0.f;

		if (m_keyboard->GetState().Escape)
		{
			PostQuitMessage(0);
		}

		float dt = GetDeltaTime();

		if (g_paused)
		{
			elapsed = 0.f;
			dt      = 0.f;
		}

		GetImGuiManager().NewFrame();

		while (elapsed >= g_fixed_update_interval)
		{
			FixedUpdate(g_fixed_update_interval);
			elapsed -= g_fixed_update_interval;
		}
		
		PreUpdate(dt);
		Update(dt);
		PostUpdate(dt);

		PreRender(dt);
		Render(dt);
		PostRender(dt);

		m_previous_keyboard_state_ = m_keyboard->GetState();
		m_previous_mouse_state_    = m_mouse->GetState();

		elapsed += dt;
	}

	void Application::SIGTERM()
	{
		TaskScheduler::Destroy();
		MouseManager::Destroy();
		Physics::CollisionDetector::Destroy();
		SceneManager::Destroy();
		ResourceManager::Destroy();
		Physics::Graviton::Destroy();
		Physics::ConstraintSolver::Destroy();
		Physics::PhysicsManager::Destroy();
		Physics::LerpManager::Destroy();
		ProjectionFrustum::Destroy();

		Graphics::ImGuiManager::Destroy();
		Graphics::ReflectionEvaluator::Destroy();
		Graphics::Renderer::Destroy();
		Graphics::RayTracer::Destroy();
		Graphics::ShadowManager::Destroy();
		Debugger::Destroy();
		Graphics::ToolkitAPI::Destroy();
		Graphics::ImGuiManager::Destroy();
		Graphics::RenderPipeline::Destroy();
		Graphics::RaytracingPipeline::Destroy();
		Graphics::D3Device::Destroy();

		Graphics::D3Device::DEBUG_MEMORY();
	}

	LRESULT Application::MessageHandler(
		HWND   hwnd, UINT msg, WPARAM wparam,
		LPARAM lparam
	)
	{
		switch (msg)
		{
		case WM_ACTIVATE:
		case WM_ACTIVATEAPP:
			Mouse::ProcessMessage(msg, wparam, lparam);
			Keyboard::ProcessMessage(msg, wparam, lparam);
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
} // namespace Engine::Manager

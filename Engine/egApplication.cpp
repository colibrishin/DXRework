#include "pch.h"
#include "egApplication.h"

#include "egGlobal.h"
#include "egManagerHelper.hpp"

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
    : Singleton() {}

  float Application::GetDeltaTime() const { return m_timer->GetElapsedSeconds(); }

  uint32_t Application::GetFPS() const { return m_timer->GetFramesPerSecond(); }

  Keyboard::State Application::GetKeyState() const { return m_keyboard->GetState(); }

  Mouse::State Application::GetMouseState() const { return m_mouse->GetState(); }

  Application::~Application()
  {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    Graphics::D3Device::DEBUG_MEMORY();
  }

  void Application::Initialize(HWND hWnd)
  {
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse    = std::make_unique<Mouse>();
    m_mouse->SetWindow(hWnd);
    m_timer = std::make_unique<DX::StepTimer>();
    UpdateWindowSize(hWnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    GetD3Device().Initialize(hWnd);
    GetToolkitAPI().Initialize();
    GetRenderPipeline().Initialize();

    GetResourceManager().Initialize();
    GetSceneManager().Initialize();
    GetDebugger().Initialize();
    GetTaskScheduler().Initialize();
    GetShadowManager().Initialize();
    GetMouseManager().Initialize();
    GetProjectionFrustum().Initialize();
    GetRenderer().Initialize();
    GetShadowManager().Initialize();
    GetReflectionEvaluator().Initialize();
    GetCollisionDetector().Initialize();
    GetLerpManager().Initialize();
    GetPhysicsManager().Initialize();
    GetConstraintSolver().Initialize();

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(GetD3Device().GetDevice(), GetD3Device().GetContext());
  }

  void Application::Tick()
  {
    static float elapsed = 0.0f;

    if (m_keyboard->GetState().Escape) { PostQuitMessage(0); }

    m_timer->Tick
      (
       [&]()
       {
         const auto dt = static_cast<float>(m_timer->GetElapsedSeconds());
         elapsed += dt;

         ImGui_ImplDX11_NewFrame();
         ImGui_ImplWin32_NewFrame();
         ImGui::NewFrame();

         PreUpdate(dt);

         if (elapsed >= g_fixed_update_interval)
         {
           FixedUpdate(dt);
           elapsed = 0.0f;
         }

         Update(dt);

         PostUpdate(dt);

         PreRender(dt);
         Render(dt);
         PostRender(dt);
       }
      );
  }

  void Application::PreUpdate(const float& dt)
  {
    GetTaskScheduler().PreUpdate(dt);
    GetMouseManager().PreUpdate(dt);
    GetCollisionDetector().PreUpdate(dt);
    GetReflectionEvaluator().PreUpdate(dt);
    GetSceneManager().PreUpdate(dt);
    GetShadowManager().PreUpdate(dt);
    GetResourceManager().PreUpdate(dt);
    GetConstraintSolver().PreUpdate(dt);
    GetPhysicsManager().PreUpdate(dt);
    GetLerpManager().PreUpdate(dt);
    GetProjectionFrustum().PreUpdate(dt);
    GetRenderer().PreUpdate(dt);
    GetShadowManager().PreUpdate(dt);
    GetDebugger().PreUpdate(dt);
    GetD3Device().PreUpdate(dt);
    GetToolkitAPI().PreUpdate(dt);
  }

  void Application::FixedUpdate(const float& dt)
  {
    GetTaskScheduler().FixedUpdate(dt);
    GetMouseManager().FixedUpdate(dt);
    GetCollisionDetector().FixedUpdate(dt);
    GetReflectionEvaluator().FixedUpdate(dt);
    GetSceneManager().FixedUpdate(dt);
    GetShadowManager().FixedUpdate(dt);
    GetResourceManager().FixedUpdate(dt);
    GetPhysicsManager().FixedUpdate(dt);
    GetConstraintSolver().FixedUpdate(dt);
    GetLerpManager().FixedUpdate(dt);
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
    GetConstraintSolver().Update(dt);
    GetPhysicsManager().Update(dt);
    GetLerpManager().Update(dt);
    GetProjectionFrustum().Update(dt);
    GetRenderer().Update(dt);
    GetShadowManager().Update(dt);
    GetDebugger().Update(dt);
    GetD3Device().Update(dt);
    GetToolkitAPI().Update(dt);
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
    GetConstraintSolver().PreRender(dt);
    GetPhysicsManager().PreRender(dt);
    GetLerpManager().PreRender(dt);
    GetProjectionFrustum().PreRender(dt);
    GetRenderer().PreRender(dt);
    GetShadowManager().PreRender(dt);
    GetDebugger().PreRender(dt);
    GetRenderPipeline().PreRender(dt);
    GetD3Device().PreRender(dt);
  }

  void Application::Render(const float& dt)
  {
    GetTaskScheduler().Render(dt);
    GetMouseManager().Render(dt);
    GetCollisionDetector().Render(dt);
    GetReflectionEvaluator().Render(dt);
    GetSceneManager().Render(dt);
    GetResourceManager().Render(dt);
    GetConstraintSolver().PreRender(dt);
    GetPhysicsManager().Render(dt);
    GetLerpManager().Render(dt);
    GetProjectionFrustum().Render(dt);
    GetRenderer().Render(dt);
    GetShadowManager().Render(dt);
    GetDebugger().Render(dt);
    GetToolkitAPI().Render(dt);
    GetD3Device().Render(dt);
  }

  void Application::PostRender(const float& dt)
  {
    GetTaskScheduler().PostRender(dt);
    GetMouseManager().PostRender(dt);
    GetCollisionDetector().PostRender(dt);
    GetReflectionEvaluator().PostRender(dt);
    GetSceneManager().PostRender(dt);
    GetResourceManager().PostRender(dt);
    GetConstraintSolver().PostRender(dt);
    GetPhysicsManager().PostRender(dt);
    GetLerpManager().PostRender(dt);
    GetProjectionFrustum().PostRender(dt);
    GetRenderer().PostRender(dt);
    GetShadowManager().PostRender(dt);
    GetDebugger().PostRender(dt);

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    GetToolkitAPI().PostRender(dt);
    GetD3Device().PostRender(dt);
  }

  void Application::PostUpdate(const float& dt)
  {
    GetTaskScheduler().PostUpdate(dt);
    GetMouseManager().PostUpdate(dt);
    GetCollisionDetector().PostUpdate(dt);
    GetReflectionEvaluator().PostUpdate(dt);
    GetSceneManager().PostUpdate(dt);
    GetResourceManager().PostUpdate(dt);
    GetConstraintSolver().PostUpdate(dt);
    GetPhysicsManager().PostUpdate(dt);
    GetLerpManager().PostUpdate(dt);
    GetProjectionFrustum().PostUpdate(dt);
    GetRenderer().PostUpdate(dt);
    GetShadowManager().PostUpdate(dt);
    GetDebugger().PostUpdate(dt);
    GetD3Device().PostUpdate(dt);
    GetToolkitAPI().PostUpdate(dt);
  }

  LRESULT Application::MessageHandler(
    HWND   hwnd, UINT msg, WPARAM wparam,
    LPARAM lparam
  )
  {
    switch (msg)
    {
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP: Mouse::ProcessMessage(msg, wparam, lparam);
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
    case WM_MOUSEHOVER: Mouse::ProcessMessage(msg, wparam, lparam);
      break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN: Keyboard::ProcessMessage(msg, wparam, lparam);
      break;

    default: return DefWindowProc(hwnd, msg, wparam, lparam);
    }
  }
} // namespace Engine::Manager

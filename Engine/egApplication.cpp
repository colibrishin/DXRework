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
      m_previous_mouse_state_(),
      m_imgui_descriptor_()
  {
    if (s_instantiated_) { throw std::runtime_error("Application is already instantiated"); }

    s_instantiated_ = true;
    std::set_terminate(SIGTERM);
  }

  float Application::GetDeltaTime() const { return static_cast<float>(m_timer->GetElapsedSeconds()); }

  uint32_t Application::GetFPS() const { return m_timer->GetFramesPerSecond(); }

  Keyboard::State Application::GetCurrentKeyState() const { return m_keyboard->GetState(); }

  bool Application::HasKeyChanged(const DirectX::Keyboard::Keys key) const
  {
    return m_previous_keyboard_state_.IsKeyUp(key) && m_keyboard->GetState().IsKeyDown(key);
  }

  bool Application::IsKeyPressed(const DirectX::Keyboard::Keys key) const
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
    else
    {
      value = 0;
      return false;
    }
  }

  Mouse::State Application::GetMouseState() const { return m_mouse->GetState(); }

  Application::~Application()
  {
    if (g_debug)
    {
      ImGui_ImplDX12_Shutdown();
      ImGui_ImplWin32_Shutdown();
      ImGui::DestroyContext();
    }
    
    Graphics::D3Device::DEBUG_MEMORY();
  }

  void Application::Initialize(HWND hWnd)
  {
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse    = std::make_unique<Mouse>();
    m_mouse->SetWindow(hWnd);
    m_timer = std::make_unique<DX::StepTimer>();
    UpdateWindowSize(hWnd);

    if constexpr (g_debug)
    {
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO& io = ImGui::GetIO();
      io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
      io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    }

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
    GetGraviton().Initialize();

    m_imgui_descriptor_ = GetRenderPipeline().AcquireHeapSlot();

    if constexpr (g_debug)
    {
      ImGui_ImplWin32_Init(hWnd);

      ImGui_ImplDX12_Init
        (
         GetD3Device().GetDevice(), g_frame_buffer, DXGI_FORMAT_R8G8B8A8_UNORM,
         GetRenderPipeline().GetDescriptor().GetBufferHeapGPU(),
         GetRenderPipeline().GetDescriptor().GetBufferHeapGPU()->GetCPUDescriptorHandleForHeapStart(),
         GetRenderPipeline().GetDescriptor().GetBufferHeapGPU()->GetGPUDescriptorHandleForHeapStart()
        );
    }
    
  }

  void Application::Tick()
  {
    m_timer->Tick([&]() { tickInternal(); });
  }

  void Application::PreUpdate(const float& dt)
  {
    GetToolkitAPI().PreUpdate(dt);
    GetGC().PreUpdate(dt);

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
    GetMouseManager().FixedUpdate(dt);
    GetReflectionEvaluator().FixedUpdate(dt);
    GetSceneManager().FixedUpdate(dt);
    GetShadowManager().FixedUpdate(dt);
    GetResourceManager().FixedUpdate(dt);

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

    GetRenderer().PreRender(dt); // pre-process render information
    GetShadowManager().PreRender(dt); // shadow resource command, executing shadow pass, set shadow resources.
    GetRenderPipeline().PreRender(dt); // clean up rtv, dsv, etc.
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

    // Shadow resource binding
    GetShadowManager().Render(dt);

    // Render commands (opaque)
    GetRenderer().Render(dt);

    GetDebugger().Render(dt);
    GetToolkitAPI().Render(dt);
    GetRenderPipeline().Render(dt);
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

    GetRenderer().PostRender(dt); // post render commands
    GetToolkitAPI().PostRender(dt); // toolkit related render commands
    GetShadowManager().PostRender(dt); // commanding shadow resource reset

    if constexpr (g_debug)
    {
      ImGui::Render();

      ImGui_ImplDX12_RenderDrawData
      (
          ImGui::GetDrawData(),
          GetD3Device().GetCommandList(COMMAND_LIST_POST_RENDER)
      );
    }

    GetReflectionEvaluator().PostRender(dt);
    GetRenderPipeline().PostRender(dt); // present
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

    if (m_keyboard->GetState().Escape) { PostQuitMessage(0); }
    const auto dt = static_cast<float>(m_timer->GetElapsedSeconds());

    if constexpr (g_debug)
    {
      ImGui_ImplDX12_NewFrame();
      ImGui_ImplWin32_NewFrame();
      ImGui::NewFrame();
    }

    PreUpdate(dt);
    Update(dt);
    PostUpdate(dt);

    if (elapsed >= g_fixed_update_interval)
    {
      FixedUpdate(elapsed);
      elapsed = std::fmod(elapsed, g_fixed_update_interval);
    }

    PreRender(dt);
    Render(dt);
    PostRender(dt);

    m_previous_keyboard_state_ = m_keyboard->GetState();
    m_previous_mouse_state_ = m_mouse->GetState();

    elapsed += dt;
  }

  void Application::SIGTERM()
  {
    GetTaskScheduler().Destroy();
    GetMouseManager().Destroy();
    GetCollisionDetector().Destroy();
    GetReflectionEvaluator().Destroy();
    GetSceneManager().Destroy();
    GetResourceManager().Destroy();
    GetGraviton().Destroy();
    GetConstraintSolver().Destroy();
    GetPhysicsManager().Destroy();
    GetLerpManager().Destroy();
    GetProjectionFrustum().Destroy();
    GetRenderer().Destroy();
    GetShadowManager().Destroy();
    GetDebugger().Destroy();
    GetD3Device().Destroy();
    GetToolkitAPI().Destroy();
    GetApplication().Destroy();
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

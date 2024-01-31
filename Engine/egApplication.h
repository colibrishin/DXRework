#pragma once
#include <memory>

#include <Keyboard.h>
#include <Mouse.h>

#include "StepTimer.hpp"
#include "egManager.hpp"

#include "imgui.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
  HWND   hWnd,
  UINT   msg,
  WPARAM wParam,
  LPARAM lParam
);

namespace Engine::Manager
{
  using DirectX::Keyboard;
  using DirectX::Mouse;

  class Application final : public Abstract::Singleton<Application, HWND>
  {
  public:
    Application(SINGLETON_LOCK_TOKEN);

    void        Initialize(HWND hwnd) override;
    static void UpdateWindowSize(HWND hWnd);
    void        Tick();

    LRESULT MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    float           GetDeltaTime() const;
    uint32_t        GetFPS() const;
    Keyboard::State GetKeyState() const;
    Mouse::State    GetMouseState() const;

  private:
    friend struct SingletonDeleter;
    ~Application() override;

    void PreUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void tickInternal();


    HWND m_hWnd = nullptr;

    // Input
    std::unique_ptr<Keyboard> m_keyboard;
    std::unique_ptr<Mouse>    m_mouse;

    // Time
    std::unique_ptr<DX::StepTimer> m_timer;
  };
} // namespace Engine::Manager

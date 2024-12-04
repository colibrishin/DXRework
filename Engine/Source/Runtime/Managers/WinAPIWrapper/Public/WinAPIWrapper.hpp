#pragma once
#include <Windows.h>
#include <memory>
#include <string>

LRESULT CALLBACK WndProc(
    HWND   hwnd, UINT umessage, WPARAM wparam,
    LPARAM lparam
);

namespace WinAPI
{
  class WinAPIWrapper final
  {
  public:
    ~WinAPIWrapper()                               = default;
    WinAPIWrapper(const WinAPIWrapper&)            = delete;
    WinAPIWrapper& operator=(const WinAPIWrapper&) = delete;

    static HWND Initialize(HINSTANCE hInstance);
    static void Update();
    static HWND GetHWND();

  private:
    WinAPIWrapper() = default;
    static HWND InitializeWindow(HINSTANCE hInstance);

    static std::unique_ptr<WinAPIWrapper> s_instance_;
    static std::wstring                   s_application_name_;
    static HINSTANCE                      s_hinstance_;
    static HWND                           s_hwnd_;
  };
} // namespace WinAPI

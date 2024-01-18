#pragma once
#include "main.h"
#include "stdafx.h"

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

  private:
    WinAPIWrapper() = default;
    static HWND InitializeWindow(HINSTANCE hInstance);

    inline static std::unique_ptr<WinAPIWrapper> s_instance_         = nullptr;
    inline static std::wstring                   s_application_name_ = L"Engine";
    inline static HINSTANCE                      s_hinstance_        = nullptr;
  };
} // namespace WinAPI

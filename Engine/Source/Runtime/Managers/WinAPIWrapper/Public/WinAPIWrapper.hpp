#pragma once
#include <Windows.h>
#include <memory>
#include <string>
#include <functional>
#include <vector>
#include "Source/Runtime/Misc.h"

namespace WinAPI
{
    LRESULT CALLBACK WndProc(
        HWND   hwnd, UINT umessage, WPARAM wparam,
        LPARAM lparam
    );
  
    class WINAPIWRAPPER_API WinAPIWrapper final
    {
    public:
        ~WinAPIWrapper()                               = default;
        WinAPIWrapper(const WinAPIWrapper&)            = delete;
        WinAPIWrapper& operator=(const WinAPIWrapper&) = delete;

        static HWND Initialize(HINSTANCE hInstance);
        static void UpdateWindowSize(const uint32_t width, const uint32_t height);
        static void Update();
        static HWND GetHWND();
        static void RegisterHandler(const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>& func);

    private:
        WinAPIWrapper() = default;
        friend LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);

        static WinAPIWrapper& GetInstance() { return *s_instance_; }

        LRESULT CALLBACK MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        static HWND InitializeWindow(HINSTANCE hInstance);

        std::vector<std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>> m_registered_handlers_;

        static std::unique_ptr<WinAPIWrapper> s_instance_;
        static std::wstring                   s_application_name_;
        static HINSTANCE                      s_hinstance_;
        static HWND                           s_hwnd_;
    };
} // namespace WinAPI

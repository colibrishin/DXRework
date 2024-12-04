#include "../Public/WinAPIWrapper.hpp"
#include "Source/Runtime/Managers/EngineEntryPoint/Public/EngineEntryPoint.h"

int WINAPI WinMain(
    HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline,
    int       iCmdshow
)
{
    // Create the system object.
    const auto hwnd = WinAPI::WinAPIWrapper::Initialize(hInstance);
    Engine::Managers::EngineEntryPoint::GetInstance().Initialize(hwnd);

    WinAPI::WinAPIWrapper::Update();
    return 0;
}

LRESULT CALLBACK WndProc(
    HWND   hwnd, UINT umessage, WPARAM wparam,
    LPARAM lparam
)
{
    switch (umessage)
    {
        // Check if the window is being destroyed.
    case WM_DESTROY:
    case WM_CLOSE:
    {
        PostQuitMessage(0);
        return 0;
    }

    // All other messages pass to the message handler in the system class.
    default: 
    { 
        return Engine::Managers::EngineEntryPoint::GetInstance().MessageHandler(hwnd, umessage, wparam, lparam);
    }
    }
}

std::unique_ptr<WinAPI::WinAPIWrapper> WinAPI::WinAPIWrapper::s_instance_ = nullptr;
std::wstring                   WinAPI::WinAPIWrapper::s_application_name_ = L"Engine";
HINSTANCE                      WinAPI::WinAPIWrapper::s_hinstance_ = nullptr;
HWND                           WinAPI::WinAPIWrapper::s_hwnd_ = nullptr;

namespace WinAPI
{
  HWND WinAPIWrapper::InitializeWindow(HINSTANCE hInstance)
  {
    WNDCLASSEXW wc{};
    DEVMODE     dmScreenSettings;
    int         posX, posY;
    UINT        initial_width  = Engine::Managers::EngineEntryPoint::GetInstance().GetWidth();
    UINT        initial_height = Engine::Managers::EngineEntryPoint::GetInstance().GetHeight();

    // Get the instance of this application.
    s_hinstance_ = GetModuleHandle(nullptr);

    // Setup the windows class with default settings.
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = s_hinstance_;
    wc.hIcon         = LoadIcon(nullptr, IDI_WINLOGO);
    wc.hIconSm       = wc.hIcon;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = s_application_name_.c_str();
    wc.cbSize        = sizeof(WNDCLASSEX);

    // Register the window class.
    RegisterClassExW(&wc);

    // Setup the screen settings depending on whether it is running in full screen
    // or in windowed mode.
    if (Engine::Managers::EngineEntryPoint::GetInstance().IsFullScreen())
    {
      // If full screen set the screen to maximum size of the users desktop and
      // 32bit.
      memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
      dmScreenSettings.dmSize      = sizeof(dmScreenSettings);
      dmScreenSettings.dmPelsWidth =
        static_cast<unsigned long>(initial_width);
      dmScreenSettings.dmPelsHeight =
        static_cast<unsigned long>(initial_height);
      dmScreenSettings.dmBitsPerPel = 32;
      dmScreenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

      // Change the display settings to full screen.
      ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

      // Set the position of the window to the top left corner.
      posX = posY = 0;
    }
    else
    {
      // Place the window in the middle of the screen.
      posX = (GetSystemMetrics(SM_CXSCREEN) - initial_width) / 2;
      posY = (GetSystemMetrics(SM_CYSCREEN) - initial_height) / 2;
    }

    // Create the window with the screen settings and get the handle to it.
    const auto hwnd = CreateWindowExW
      (
       WS_EX_APPWINDOW, s_application_name_.c_str(), s_application_name_.c_str(),
       WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP, posX, posY,
       initial_width, initial_height, nullptr, nullptr,
       s_hinstance_, nullptr
      );

    // Bring the window up on the screen and set it as main focus.
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    ShowCursor(false);


    // Show mouse cursor for debugging.
    ShowCursor(true);

    s_hwnd_ = hwnd;

    return hwnd;
  }

  HWND WinAPIWrapper::Initialize(HINSTANCE hInstance)
  {
    s_instance_ = std::unique_ptr<WinAPIWrapper>(new WinAPIWrapper());
    return InitializeWindow(hInstance);
  }

  void WinAPIWrapper::Update()
  {
    MSG msg;
    // Initialize the message structure.
    ZeroMemory(&msg, sizeof(MSG));

    // Handle the windows messages.
    while (true)
    {
      if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_QUIT) 
        { 
            return; 
        }
      }
      else 
      { 
          Engine::Managers::EngineEntryPoint::GetInstance().Tick();
      }
    }
  }
  HWND WinAPIWrapper::GetHWND()
  {
      return s_hwnd_;
  }
} // namespace WinAPI

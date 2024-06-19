#include "WinAPIWrapper.hpp"
#include "stdafx.h"
#include "../Client/Client.h"

#include "../Engine/egDebugConstant.h"

int WINAPI WinMain(
  HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline,
  int       iCmdshow
)
{
  // Create the system object.
  const auto hwnd = WinAPI::WinAPIWrapper::Initialize(hInstance);
  Client::Initialize(hwnd);

  WinAPI::WinAPIWrapper::Update();

  return 0;
}

LRESULT CALLBACK WndProc(
  HWND   hwnd, UINT umessage, WPARAM wparam,
  LPARAM lparam
)
{
  if (ImGui_ImplWin32_WndProcHandler(hwnd, umessage, wparam, lparam)) { return true; }

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
  default: { return Client::MessageHandler(hwnd, umessage, wparam, lparam); }
  }
}

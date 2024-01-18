#pragma once
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

LRESULT CALLBACK WndProc(
  HWND   hwnd, UINT umessage, WPARAM wparam,
  LPARAM lparam
);

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

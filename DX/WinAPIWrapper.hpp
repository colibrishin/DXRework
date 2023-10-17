#pragma once
#include "main.h"
#include "../Engine/egApplication.hpp"

namespace WinAPI
{
	class WinAPIWrapper final
	{
	public:
		~WinAPIWrapper() = default;
		WinAPIWrapper(const WinAPIWrapper&) = delete;
		WinAPIWrapper& operator=(const WinAPIWrapper&) = delete;

		static HWND Initialize(HINSTANCE hInstance);
		static void Update();

	private:
		WinAPIWrapper() = default;
		static HWND InitializeWindow(HINSTANCE hInstance);

		inline static std::unique_ptr<WinAPIWrapper> s_instance_ = nullptr;
		inline static std::wstring s_application_name_ = L"Engine";
		inline static HINSTANCE s_hinstance_ = nullptr;

	};

	inline HWND WinAPIWrapper::InitializeWindow(HINSTANCE hInstance)
	{
		WNDCLASSEXW wc{};
		DEVMODE dmScreenSettings;
		int posX, posY;

		// Get the instance of this application.
		s_hinstance_ = GetModuleHandle(NULL);

		// Setup the windows class with default settings.
		wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc   = WndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = s_hinstance_;
		wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
		wc.hIconSm       = wc.hIcon;
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = s_application_name_.c_str();
		wc.cbSize        = sizeof(WNDCLASSEX);
		
		// Register the window class.
		RegisterClassEx(&wc);

		// Setup the screen settings depending on whether it is running in full screen or in windowed mode.
		if(Engine::g_full_screen)
		{
			// If full screen set the screen to maximum size of the users desktop and 32bit.
			memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
			dmScreenSettings.dmSize       = sizeof(dmScreenSettings);
			dmScreenSettings.dmPelsWidth  = (unsigned long)Engine::g_window_width;
			dmScreenSettings.dmPelsHeight = (unsigned long)Engine::g_window_height;
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
			posX = (GetSystemMetrics(SM_CXSCREEN) - Engine::g_window_width) / 2;
			posY = (GetSystemMetrics(SM_CYSCREEN) - Engine::g_window_height) / 2;
		}

		// Create the window with the screen settings and get the handle to it.
		const auto hwnd = ::CreateWindowExW(WS_EX_APPWINDOW, s_application_name_.c_str(), s_application_name_.c_str(), 
		                           WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
		                           posX, posY, Engine::g_window_width, Engine::g_window_height, NULL, NULL, s_hinstance_, NULL);

		// Bring the window up on the screen and set it as main focus.
		ShowWindow(hwnd, SW_SHOW);
		SetForegroundWindow(hwnd);
		SetFocus(hwnd);

		// Hide the mouse cursor.
		ShowCursor(false);

		return hwnd;
	}

	inline HWND WinAPIWrapper::Initialize(HINSTANCE hInstance)
	{
		s_instance_ = std::unique_ptr<WinAPIWrapper>(new WinAPIWrapper());
		return InitializeWindow(hInstance);
	}

	inline void WinAPIWrapper::Update()
	{
		MSG msg;
		// Initialize the message structure.
		ZeroMemory(&msg, sizeof(MSG));

		// Handle the windows messages.
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

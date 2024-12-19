#include "../Public/WinAPIWrapper.hpp"
#include "Source/Runtime/Managers/EngineEntryPoint/Public/EngineEntryPoint.h"

int WINAPI WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline,
	int       iCmdshow
)
{
	// Create the system object.
	const auto hwnd = WinAPI::WinAPIWrapper::Initialize(hInstance);
	Engine::Managers::EngineEntryPoint::GetInstance().Initialize();

	WinAPI::WinAPIWrapper::Update();
	return 0;
}

std::unique_ptr<WinAPI::WinAPIWrapper> WinAPI::WinAPIWrapper::s_instance_         = nullptr;
std::wstring                           WinAPI::WinAPIWrapper::s_application_name_ = L"Engine";
HINSTANCE                              WinAPI::WinAPIWrapper::s_hinstance_        = nullptr;
HWND                                   WinAPI::WinAPIWrapper::s_hwnd_             = nullptr;

namespace WinAPI
{
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
			return WinAPIWrapper::GetInstance().MessageHandler(hwnd, umessage, wparam, lparam);
		}
		}
	}

	LRESULT WinAPIWrapper::MessageHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		for (const auto& func : m_registered_handlers_)
		{
			func(hwnd, msg, wparam, lparam);
		}

		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	HWND WinAPIWrapper::InitializeWindow(HINSTANCE hInstance)
	{
		WNDCLASSEXW wc{};
		DEVMODE     dmScreenSettings;
		int         posX, posY;
		UINT        initial_width  = CFG_WIDTH;
		UINT        initial_height = CFG_HEIGHT;

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
		if (CFG_FULLSCREEN)
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

	void WinAPIWrapper::UpdateWindowSize(const uint32_t width, const uint32_t height)
	{
		SetWindowPos
				(
				 s_hwnd_, nullptr,
				 (GetSystemMetrics(SM_CXSCREEN) - width) / 2,
				 (GetSystemMetrics(SM_CYSCREEN) - height) / 2,
				 width, height, SWP_NOMOVE | SWP_NOZORDER
				);

		ShowWindow(s_hwnd_, SW_SHOW);
		SetForegroundWindow(s_hwnd_);
		SetFocus(s_hwnd_);
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

	void WinAPIWrapper::RegisterHandler(const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>& func)
	{
		GetInstance().m_registered_handlers_.push_back(func);
	}
} // namespace WinAPI

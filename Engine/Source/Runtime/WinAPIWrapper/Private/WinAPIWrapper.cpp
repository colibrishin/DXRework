#include "WinAPIWrapper.hpp"
#include "EngineEntryPoint.h"

std::unique_ptr<WinAPI::WinAPIWrapper> WinAPI::WinAPIWrapper::s_instance_         = nullptr;
std::wstring                           WinAPI::WinAPIWrapper::s_application_name_ = L"Engine";
HINSTANCE                              WinAPI::WinAPIWrapper::s_hinstance_        = nullptr;
HWND                                   WinAPI::WinAPIWrapper::s_hwnd_             = nullptr;
bool                                   WinAPI::WinAPIWrapper::s_alt_pressed_      = false;

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
		case WM_SYSKEYDOWN:
		{
            if ( wparam == VK_MENU )
            {
                WinAPIWrapper::s_alt_pressed_ = true;
            }
			break;
		}
		case WM_SYSKEYUP:
		{
            if ( wparam == VK_MENU )
            {
                WinAPIWrapper::s_alt_pressed_ = false;
            }
			break;
		}
		case WM_KEYDOWN:
		{
            if ( wparam == VK_F4 && WinAPIWrapper::s_alt_pressed_ )
            {
                PostQuitMessage( 0 );
                return 0;
            }
			break;
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
		for (const auto& [name, func] : m_registered_handlers_)
		{
			func(hwnd, msg, wparam, lparam);
		}

		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	HWND WinAPIWrapper::InitializeWindow(HINSTANCE hInstance)
	{
#if SERVER
        if (AllocConsole())
        {
            static FILE* file;
            if ( freopen_s( &file, "CONOUT$", "w", stdout ) || freopen_s( &file, "CONOUT$", "w", stderr ) )
            {
                assert( nullptr );
            }
        }
#endif

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
#if SERVER
	    ShowWindow( hwnd, SW_HIDE );
		SetFocus(hwnd);
        ShowCursor( true );
#else
	    ShowWindow(hwnd, SW_SHOW);
		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
		ShowCursor(false);
#endif

#if WITH_DEBUG
		// Show mouse cursor for debugging.
		ShowCursor(true);
#endif

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
                    Engine::Managers::EngineEntryPoint::GetInstance().Destroy();
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

	void WinAPIWrapper::RegisterHandler(const std::string_view name, const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>& func)
	{
		GetInstance().m_registered_handlers_.emplace_back(name.data(), func);
	}

	void WinAPIWrapper::UnregisterHandler(const std::string_view name)
	{
		std::erase_if(GetInstance().m_registered_handlers_, [&name](const auto& pair)
			{
				return pair.first == name;
			});
	}
} // namespace WinAPI

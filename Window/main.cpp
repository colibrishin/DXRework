#include "stdafx.h"
#include "../Engine/egApplication.hpp"
#include "WinAPIWrapper.hpp"
#include "../Client/Client.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	// Create the system object.
	const auto hwnd = WinAPI::WinAPIWrapper::Initialize(hInstance);
	Engine::Application::Initialize(hwnd);
	Client::fnClient();

	WinAPI::WinAPIWrapper::Update();

	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch(umessage)
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
			return Engine::Application::MessageHandler(hwnd, umessage, wparam, lparam);
		}
	}
}
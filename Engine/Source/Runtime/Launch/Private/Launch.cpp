#include "WinAPIWrapper.hpp"
#include "EngineEntryPoint.h"

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
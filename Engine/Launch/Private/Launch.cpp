#ifdef CFG_MONOLITH
#include "Monolith/Monolith.h"
#else
#include "WinAPIWrapper.hpp"
#include "EngineEntryPoint.h"
#endif

#if Platform == Windows
int WINAPI WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline,
	int       iCmdshow
)
{
	// Create the system object.
#ifndef CFG_MONOLITH
	const auto hwnd = WinAPI::WinAPIWrapper::Initialize(hInstance);
    Engine::Managers::EngineEntryPoint::GetInstance().Initialize();
    WinAPI::WinAPIWrapper::Update();
#else
	MonolithicLaunch(hInstance);
#endif
	return 0;
}
#endif
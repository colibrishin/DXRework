#include "Monolith.h"
#include "WinAPIWrapper.hpp"
#include "EngineEntryPoint.h"

void MonolithicLaunch( HINSTANCE hInstance )
{
    WinAPI::WinAPIWrapper::Initialize( hInstance );
    Engine::Managers::EngineEntryPoint::GetInstance().Initialize();
    WinAPI::WinAPIWrapper::Update();
}

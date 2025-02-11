#include "WinAPIWrapperModule.h"
#include "WinAPIWrapperModule.generated.h"

#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::WinAPIWrapperModule, WinAPIWrapper)

bool Engine::WinAPIWrapperModule::InitializeImpl()
{
    return true;
}

bool Engine::WinAPIWrapperModule::ShutdownImpl()
{
    return true;
}

bool Engine::WinAPIWrapperModule::DynamicLoadable()
{
    return true;
}
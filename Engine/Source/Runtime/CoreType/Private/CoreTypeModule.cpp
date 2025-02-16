#include "CoreTypeModule.h"
#include "CoreTypeModule.generated.h"

MODULE_IMPL(Engine::CoreTypeModule, CoreType)

bool Engine::CoreTypeModule::InitializeImpl()
{
    return true;
}

bool Engine::CoreTypeModule::ShutdownImpl()
{
    return true;
}

bool Engine::CoreTypeModule::DynamicLoadable()
{
    return true;
}

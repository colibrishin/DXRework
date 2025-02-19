#include "CoreEntityModule.h"
#include "CoreEntityModule.generated.h"

MODULE_IMPL(Engine::CoreEntityModule, CoreEntity)

bool Engine::CoreEntityModule::InitializeImpl()
{
    return true;
}

bool Engine::CoreEntityModule::ShutdownImpl()
{
    return true;
}

bool Engine::CoreEntityModule::DynamicLoadable()
{
    return true;
}

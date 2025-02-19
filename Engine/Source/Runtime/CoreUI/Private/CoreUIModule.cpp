#include "CoreUIModule.h"
#include "CoreUIModule.generated.h"

MODULE_IMPL( Engine::CoreUIModule, CoreUI )

bool Engine::CoreUIModule::InitializeImpl()
{
    return true;
}

bool Engine::CoreUIModule::ShutdownImpl()
{
    return true;
}

bool Engine::CoreUIModule::DynamicLoadable()
{
    return true;
}

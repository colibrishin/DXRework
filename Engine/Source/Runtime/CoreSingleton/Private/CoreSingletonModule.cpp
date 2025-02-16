#include "CoreSingletonModule.h"
#include "CoreSingletonModule.generated.h"

MODULE_IMPL( Engine::CoreSingletonModule, CoreSingleton )

bool Engine::CoreSingletonModule::InitializeImpl()
{
    return true;
}

bool Engine::CoreSingletonModule::ShutdownImpl()
{
    return true;
}

bool Engine::CoreSingletonModule::DynamicLoadable()
{
    return true;
}

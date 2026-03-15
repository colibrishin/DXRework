#include "DirectInputInterfaceModule.h"

#include "IInputAPI.h"
#include "DirectInputInterface.h"

MODULE_IMPL( Engine::DirectInputInterfaceModule, DirectInputInterface )

bool Engine::DirectInputInterfaceModule::InitializeImpl()
{
    g_input_accessor.SetInterface<DirectInputInterface>();
    return true;
}

bool Engine::DirectInputInterfaceModule::ShutdownImpl()
{
    g_input_accessor.Shutdown();
    return true;
}

bool Engine::DirectInputInterfaceModule::DynamicLoadable()
{
    return true;
}

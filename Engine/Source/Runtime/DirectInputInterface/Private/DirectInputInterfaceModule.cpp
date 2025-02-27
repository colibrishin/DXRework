#include "DirectInputInterfaceModule.h"
#include "DirectInputInterfaceModule.generated.h"

#include "IInputAPI.h"
#include "DirectInputInterface.h"

MODULE_IMPL( Engine::DirectInputInterfaceModule, DirectInputInterface )

bool Engine::DirectInputInterfaceModule::InitializeImpl()
{
    s_iia.SetInterface<DirectInputInterface>();
    return true;
}

bool Engine::DirectInputInterfaceModule::ShutdownImpl()
{
    s_iia.Shutdown();
    return true;
}

bool Engine::DirectInputInterfaceModule::DynamicLoadable()
{
    return true;
}

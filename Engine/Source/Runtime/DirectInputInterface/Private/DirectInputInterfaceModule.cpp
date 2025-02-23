#include "DirectInputInterfaceModule.h"
#include "DirectInputInterfaceModule.generated.h"

#include "InputInterface.h"
#include "DirectInputInterface.h"

MODULE_IMPL( Engine::DirectInputInterfaceModule, DirectInputInterface )

bool Engine::DirectInputInterfaceModule::InitializeImpl()
{
    InputInterfaceAccessor::SetInterface<DirectInputInterface>();
    return true;
}

bool Engine::DirectInputInterfaceModule::ShutdownImpl()
{
    return true;
}

bool Engine::DirectInputInterfaceModule::DynamicLoadable()
{
    return true;
}

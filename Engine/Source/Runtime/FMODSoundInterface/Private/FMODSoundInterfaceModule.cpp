#include "FMODSoundInterfaceModule.h"
#include "FMODSoundInterfaceModule.generated.h"

#include "FMODSoundInterface.h"
#include "ISoundAPI.h"


MODULE_IMPL(Engine::FMODSoundInterfaceModule, FMODSoundInterface)

bool Engine::FMODSoundInterfaceModule::InitializeImpl()
{
    s_sa.SetInterface<FMODSoundInterface>();
    return true;
}

bool Engine::FMODSoundInterfaceModule::ShutdownImpl()
{
    s_sa.Shutdown();
    return true;
}

bool Engine::FMODSoundInterfaceModule::DynamicLoadable()
{
    return true;
}

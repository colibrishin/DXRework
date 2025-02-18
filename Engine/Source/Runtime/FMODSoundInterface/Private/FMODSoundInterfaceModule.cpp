#include "FMODSoundInterfaceModule.h"
#include "FMODSoundInterfaceModule.generated.h"

#include "FMODSoundInterface.h"
#include "SoundInterface.h"


MODULE_IMPL(Engine::FMODSoundInterfaceModule, FMODSoundInterface)

bool Engine::FMODSoundInterfaceModule::InitializeImpl()
{
    SoundInterfaceAccessor::SetInterface<FMODSoundInterface>();
    return true;
}

bool Engine::FMODSoundInterfaceModule::ShutdownImpl()
{
    SoundInterfaceAccessor::Shutdown();
    return true;
}

bool Engine::FMODSoundInterfaceModule::DynamicLoadable()
{
    return true;
}

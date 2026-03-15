#include "FMODSoundInterfaceModule.h"

#include "FMODSoundInterface.h"
#include "ISoundAPI.h"


MODULE_IMPL(Engine::FMODSoundInterfaceModule, FMODSoundInterface)

bool Engine::FMODSoundInterfaceModule::InitializeImpl()
{
    g_sound_accessor.SetInterface<FMODSoundInterface>();
    return true;
}

bool Engine::FMODSoundInterfaceModule::ShutdownImpl()
{
    g_sound_accessor.Shutdown();
    return true;
}

bool Engine::FMODSoundInterfaceModule::DynamicLoadable()
{
    return true;
}

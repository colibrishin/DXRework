#include "SoundManagerModule.h"

#include "EngineEntryPoint.h"
#include "SoundManagerModule.generated.h"

#include "ISoundAPI.h"
#include "SoundManager.h"

MODULE_IMPL(Engine::SoundManagerModule, SoundManager)

bool Engine::SoundManagerModule::InitializeImpl()
{
    CoreLoop::AddManager(
        CoreLoop::LOOP_TYPE_LOGIC,
        &Managers::SoundManager::GetInstance);
    return true;
}

bool Engine::SoundManagerModule::ShutdownImpl()
{
    CoreLoop::RemoveManager(
        CoreLoop::LOOP_TYPE_LOGIC,
        &Managers::SoundManager::GetInstance);
    g_sound_accessor.Shutdown();
    return true;
}

bool Engine::SoundManagerModule::DynamicLoadable()
{
    return true;
}

const std::vector<std::string>& Engine::SoundManagerModule::LoadAfter() const
{
    static std::vector<std::string> load_after{ "FMODSoundInterface" };
    return load_after;
}

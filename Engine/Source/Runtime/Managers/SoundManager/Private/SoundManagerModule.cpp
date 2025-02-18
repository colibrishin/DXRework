#include "SoundManagerModule.h"
#include "SoundManagerModule.generated.h"

#include "SoundManager.h"

#include "CoreModule/Public/CoreModule.h"


MODULE_IMPL(Engine::SoundManagerModule, SoundManager)

bool Engine::SoundManagerModule::InitializeImpl()
{
    CoreModule::GetContext().AddManager(
        CoreLoop::LOOP_TYPE_LOGIC,
        &Managers::SoundManager::GetInstance);
    return true;
}

bool Engine::SoundManagerModule::ShutdownImpl()
{
    CoreModule::GetContext().RemoveManager(
        CoreLoop::LOOP_TYPE_LOGIC,
        &Managers::SoundManager::GetInstance);
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

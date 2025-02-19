#include "ShadowManagerModule.h"
#include "ShadowManagerModule.generated.h"

#include "ShadowManager.h"

#include "CoreModule/Public/CoreModule.h"



MODULE_IMPL(Engine::ShadowManagerModule, ShadowManager)

bool Engine::ShadowManagerModule::InitializeImpl()
{
    CoreModule::GetContext().AddManager(
        CoreLoop::LOOP_TYPE_RENDER,
        &Managers::ShadowManager::GetInstance);

    return true;
}

bool Engine::ShadowManagerModule::ShutdownImpl()
{
    CoreModule::GetContext().RemoveManager(
        CoreLoop::LOOP_TYPE_RENDER,
        &Managers::ShadowManager::GetInstance);

    return true;
}

bool Engine::ShadowManagerModule::DynamicLoadable()
{
    return true;
}

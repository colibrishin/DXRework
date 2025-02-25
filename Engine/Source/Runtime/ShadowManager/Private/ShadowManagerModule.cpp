#include "ShadowManagerModule.h"

#include "EngineEntryPoint.h"
#include "ShadowManagerModule.generated.h"

#include "ShadowManager.h"

MODULE_IMPL(Engine::ShadowManagerModule, ShadowManager)

bool Engine::ShadowManagerModule::InitializeImpl()
{
    CoreLoop::AddManager(
        CoreLoop::LOOP_TYPE_RENDER,
        &Managers::ShadowManager::GetInstance);

    return true;
}

bool Engine::ShadowManagerModule::ShutdownImpl()
{
    CoreLoop::RemoveManager(
        CoreLoop::LOOP_TYPE_RENDER,
        &Managers::ShadowManager::GetInstance);

    return true;
}

bool Engine::ShadowManagerModule::DynamicLoadable()
{
    return true;
}

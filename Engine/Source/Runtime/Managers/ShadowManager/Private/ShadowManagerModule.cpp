#include "ShadowManagerModule.h"
#include "ShadowManagerModule.generated.h"

#include "ShadowManager.h"

#include "CoreModuel/Public/CoreModule.h"

#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::ShadowManagerModule, ShadowManager)

void Engine::ShadowManagerModule::Initialize()
{
    CoreModule::GetContext().AddManager(
        CoreLoop::LOOP_TYPE_RENDER,
        &Managers::ShadowManager::GetInstance);
}

void Engine::ShadowManagerModule::Shutdown()
{
    CoreModule::GetContext().RemoveManager(
        CoreLoop::LOOP_TYPE_RENDER,
        &Managers::ShadowManager::GetInstance);
}

bool Engine::ShadowManagerModule::DynamicLoadable()
{
    return true;
}

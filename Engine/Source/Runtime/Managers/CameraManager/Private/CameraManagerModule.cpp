#include "CameraManagerModule.h"
#include "CameraManagerModule.generated.h"

#include "CameraManager.h"

#include "CoreModuel/Public/CoreModule.h"

#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::CameraManagerModule, CameraManager)

void Engine::CameraManagerModule::Initialize()
{
    CoreModule::GetContext().AddManager(
        CoreLoop::LOOP_TYPE_LOGIC,
        &Managers::CameraManager::GetInstance);
}

void Engine::CameraManagerModule::Shutdown()
{
    CoreModule::GetContext().RemoveManager(
        CoreLoop::LOOP_TYPE_LOGIC,
        &Managers::CameraManager::GetInstance);
}

bool Engine::CameraManagerModule::DynamicLoadable()
{
    return true;
}

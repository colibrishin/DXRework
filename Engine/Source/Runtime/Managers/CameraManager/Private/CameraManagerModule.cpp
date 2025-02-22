#include "CameraManagerModule.h"
#include "CameraManagerModule.generated.h"

#include "CameraManager.h"
#include "CoreModule/Public/CoreModule.h"

MODULE_IMPL( Engine::CameraManagerModule, CameraManager )

bool Engine::CameraManagerModule::InitializeImpl()
{
    CoreModule::GetContext().AddManager(
        CoreLoop::LOOP_TYPE_LOGIC,
        &Managers::CameraManager::GetInstance);

    return true;
}

bool Engine::CameraManagerModule::ShutdownImpl()
{
    CoreModule::GetContext().RemoveManager(
        CoreLoop::LOOP_TYPE_LOGIC,
        &Managers::CameraManager::GetInstance);

    return true;
}

bool Engine::CameraManagerModule::DynamicLoadable()
{
    return true;
}

#include "CameraManagerModule.h"
#include "CameraManagerModule.generated.h"

#include "CameraManager.h"
#include "EngineEntryPoint.h"
#include "CoreModule.h"

MODULE_IMPL( Engine::CameraManagerModule, CameraManager )

bool Engine::CameraManagerModule::InitializeImpl()
{
    CoreLoop::AddManager(
        CoreLoop::LOOP_TYPE_LOGIC,
        &Managers::CameraManager::GetInstance);

    return true;
}

bool Engine::CameraManagerModule::ShutdownImpl()
{
    CoreLoop::RemoveManager(
        CoreLoop::LOOP_TYPE_LOGIC,
        &Managers::CameraManager::GetInstance);

    return true;
}

bool Engine::CameraManagerModule::DynamicLoadable()
{
    return true;
}

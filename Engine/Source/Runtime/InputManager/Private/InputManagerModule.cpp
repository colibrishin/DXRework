#include "InputManagerModule.h"

#include <d3d12.h>

#include "EngineEntryPoint.h"
#include "InputManager.h"

MODULE_IMPL( Engine::InputManagerModule, InputManager )

bool Engine::InputManagerModule::InitializeImpl()
{
    CoreLoop::AddManager(CoreLoop::LOOP_TYPE_LOGIC, &Managers::InputManager::GetInstance);
    return true;
}

bool Engine::InputManagerModule::ShutdownImpl()
{
    CoreLoop::RemoveManager(CoreLoop::LOOP_TYPE_LOGIC, &Managers::InputManager::GetInstance);
    g_input_accessor.Shutdown();
    return true;
}

bool Engine::InputManagerModule::DynamicLoadable()
{
    return true;
}

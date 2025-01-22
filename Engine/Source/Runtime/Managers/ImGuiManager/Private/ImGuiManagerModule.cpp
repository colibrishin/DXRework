#include "ImGuiManager.h"
#include "ImGuiManagerModule.h"
#include "ModuleManager/Public/ModuleManager.h"

#include "CoreModuel/Public/CoreModule.h"

MODULE_IMPL(Engine::ImGuiManagerModule, ImGuiManager)

void Engine::ImGuiManagerModule::Initialize()
{
	CoreModule::GetContext().AddManager(
		CoreLoop::LOOP_TYPE_RENDER,
		Managers::ImGuiManager::GetInstance);
}

void Engine::ImGuiManagerModule::Shutdown()
{
	CoreModule::GetContext().RemoveManager(
		CoreLoop::LOOP_TYPE_RENDER,
		Managers::ImGuiManager::GetInstance);
}

bool Engine::ImGuiManagerModule::DynamicLoadable()
{
	return true;	
}

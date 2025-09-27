#include "ImGuiManager.h"
#include "ImGuiManagerModule.h"
#include "ImGuiManagerModule.generated.h"

#include "EngineEntryPoint.h"
#include "CoreModule.h"


MODULE_IMPL(Engine::ImGuiManagerModule, ImGuiManager)

bool Engine::ImGuiManagerModule::InitializeImpl()
{
	CoreLoop::AddManager(
		CoreLoop::LOOP_TYPE_RENDER,
		Managers::ImGuiManager::GetInstance);
	return true;
}

bool Engine::ImGuiManagerModule::ShutdownImpl()
{
	CoreLoop::RemoveManager(
		CoreLoop::LOOP_TYPE_RENDER,
		Managers::ImGuiManager::GetInstance);
	return true;
}

bool Engine::ImGuiManagerModule::DynamicLoadable()
{
	return true;	
}

const std::vector<std::string>& Engine::ImGuiManagerModule::LoadAfter() const
{
	static std::vector<std::string> load_after = { "RenderPipeline" };
	return load_after;
}

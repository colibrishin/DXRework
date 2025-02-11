#include "ImGuiManager.h"
#include "ImGuiManagerModule.h"
#include "ImGuiManagerModule.generated.h"
#include "ModuleManager/Public/ModuleManager.h"

#include "CoreModuel/Public/CoreModule.h"

MODULE_IMPL(Engine::ImGuiManagerModule, ImGuiManager)

bool Engine::ImGuiManagerModule::InitializeImpl()
{
	CoreModule::GetContext().AddManager(
		CoreLoop::LOOP_TYPE_RENDER,
		Managers::ImGuiManager::GetInstance);

	return true;
}

bool Engine::ImGuiManagerModule::ShutdownImpl()
{
	CoreModule::GetContext().RemoveManager(
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
	static std::vector<std::string> load_after = {
		"RenderPipeline",
#if USE_DX12
		"D3D12GraphicInterface",
#endif
#if Platform == Windows
		"WinAPIWrapper",
#endif
	};

	return load_after;
}

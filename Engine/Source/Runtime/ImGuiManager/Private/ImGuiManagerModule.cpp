#include "ImGuiManager.h"
#include "ImGuiManagerModule.h"

#include "EngineEntryPoint.h"
#include "ImGuiManagerModule.generated.h"


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

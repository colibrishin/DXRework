#include "RenderPipelineModule.h"
#include "RenderPipelineModule.generated.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "CoreModuel/Public/CoreModule.h"
#include "RenderPipeline.h"
#include "Renderer.h"

MODULE_IMPL(Engine::RenderPipelineModule, RenderPipeline)

bool Engine::RenderPipelineModule::InitializeImpl()
{
	CoreModule::GetContext().AddManager(
		CoreLoop::LOOP_TYPE_RENDER,
		&Managers::RenderPipeline::GetInstance,
		&Managers::Renderer::GetInstance);
	return true;
}

bool Engine::RenderPipelineModule::ShutdownImpl()
{
	CoreModule::GetContext().RemoveManager(
		CoreLoop::LOOP_TYPE_RENDER,
		&Managers::RenderPipeline::GetInstance,
		&Managers::Renderer::GetInstance);

	return true;
}

bool Engine::RenderPipelineModule::DynamicLoadable()
{
	return true;
}

const std::vector<std::string>& Engine::RenderPipelineModule::LoadAfter() const
{
#if USE_DX12
	static std::vector<std::string> load_after = { "D3D12GraphicInterface" };
#endif
	return load_after;
}

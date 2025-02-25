#include "RenderPipelineModule.h"

#include "EngineEntryPoint.h"
#include "RenderPipelineModule.generated.h"

#include "RenderPipeline.h"
#include "Renderer.h"

MODULE_IMPL(Engine::RenderPipelineModule, RenderPipeline)

bool Engine::RenderPipelineModule::InitializeImpl()
{
	CoreLoop::AddManager(
		CoreLoop::LOOP_TYPE_RENDER,
		&Managers::RenderPipeline::GetInstance,
		&Managers::Renderer::GetInstance);
	return true;
}

bool Engine::RenderPipelineModule::ShutdownImpl()
{
	CoreLoop::RemoveManager(
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
	static const std::vector<std::string> load_after = { 	
#if USE_DX12
		"D3D12GraphicInterface", 
#endif 
	};
	return load_after;
}

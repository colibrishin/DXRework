#include "RenderPipelineModule.h"

#include "EngineEntryPoint.h"

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

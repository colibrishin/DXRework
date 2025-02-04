#include "../Public/RaytracingRenderPassTaskModule.h"
#include "Renderer.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "RaytracingRenderPassTask.h"

MODULE_IMPL(Engine::RaytracingRenderPassTaskModule, RaytracingRenderPassTask)

namespace Engine
{
	void RaytracingRenderPassTaskModule::Initialize()
	{
		Managers::Renderer::GetInstance().RegisterRenderPass(L"RaytracingRenderPassTask", new RaytracingRenderPassTask());
	}

	void RaytracingRenderPassTaskModule::Shutdown()
	{
		Managers::Renderer::GetInstance().UnregisterRenderPass(L"RaytracingRenderPassTask");
	}

	bool RaytracingRenderPassTaskModule::DynamicLoadable()
	{
		return true;
	}
}

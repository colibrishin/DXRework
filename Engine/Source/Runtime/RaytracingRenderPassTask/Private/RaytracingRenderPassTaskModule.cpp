#include "../Public/RaytracingRenderPassTaskModule.h"
#include "RaytracingRenderPassTaskModule.generated.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "RaytracingRenderPassTask.h"
#include "RaytracingRenderer.h"

MODULE_IMPL(Engine::RaytracingRenderPassTaskModule, RaytracingRenderPassTask)

namespace Engine
{
	void RaytracingRenderPassTaskModule::Initialize()
	{
		Managers::RaytracingRenderer::GetInstance().RegisterRenderPass(L"RaytracingRenderPassTask", new RaytracingRenderPassTask());
	}

	void RaytracingRenderPassTaskModule::Shutdown()
	{
		Managers::RaytracingRenderer::GetInstance().UnregisterRenderPass(L"RaytracingRenderPassTask");
	}

	bool RaytracingRenderPassTaskModule::DynamicLoadable()
	{
		return true;
	}
}

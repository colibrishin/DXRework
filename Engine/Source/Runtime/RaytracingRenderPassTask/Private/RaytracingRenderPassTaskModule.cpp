#include "../Public/RaytracingRenderPassTaskModule.h"
#include "RaytracingRenderPassTaskModule.generated.h"

#include "ModuleManager.h"
#include "RaytracingRenderPassTask.h"
#include "RaytracingRenderer.h"

MODULE_IMPL(Engine::RaytracingRenderPassTaskModule, RaytracingRenderPassTask)

namespace Engine
{
	bool RaytracingRenderPassTaskModule::InitializeImpl()
	{
#if CFG_RAYTRACING
		Managers::RaytracingRenderer::GetInstance().RegisterRenderPass(L"RaytracingRenderPassTask", new RaytracingRenderPassTask());
#endif
	    return true;
	}

	bool RaytracingRenderPassTaskModule::ShutdownImpl()
	{
#if CFG_RAYTRACING
		Managers::RaytracingRenderer::GetInstance().UnregisterRenderPass(L"RaytracingRenderPassTask");
#endif
	    return true;
	}

	bool RaytracingRenderPassTaskModule::DynamicLoadable()
	{
		return true;
	}
}

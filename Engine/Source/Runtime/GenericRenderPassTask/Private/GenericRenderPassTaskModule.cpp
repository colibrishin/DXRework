#include "../Public/GenericRenderPassTaskModule.h"
#include "Renderer.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "GenericRenderPassTask.h"

MODULE_IMPL(Engine::GenericRenderPassTaskModule, GenericRenderPassTask)

namespace Engine
{
	void GenericRenderPassTaskModule::Initialize()
	{
		Managers::Renderer::GetInstance().RegisterRenderPass(L"GenericRenderPassTask", new GenericRenderPassTask());
	}

	void GenericRenderPassTaskModule::Shutdown()
	{
		Managers::Renderer::GetInstance().UnregisterRenderPass(L"GenericRenderPassTask");
	}

	bool GenericRenderPassTaskModule::DynamicLoadable()
	{
		return true;
	}
}

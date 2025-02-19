#include "GenericRenderPassTaskModule.h"
#include "GenericRenderPassTaskModule.generated.h"
#include "Renderer.h"


#include "GenericRenderPassTask.h"

MODULE_IMPL(Engine::GenericRenderPassTaskModule, GenericRenderPassTask)

namespace Engine
{
	bool GenericRenderPassTaskModule::InitializeImpl()
	{
		Managers::Renderer::GetInstance().RegisterRenderPass(L"GenericRenderPassTask", new GenericRenderPassTask());

		return true;
	}

	bool GenericRenderPassTaskModule::ShutdownImpl()
	{
		Managers::Renderer::GetInstance().UnregisterRenderPass(L"GenericRenderPassTask");

		return true;
	}

	bool GenericRenderPassTaskModule::DynamicLoadable()
	{
		return true;
	}
}

#pragma once
#include "CoreType.h"
#include "ModuleManager.h"

#include "ForwardRenderPassTaskModule.generated.h"

namespace Engine 
{
	ECLASS(module)
    struct ENGINE_FORWARDRENDERPASSTASK_API ForwardRenderPassTaskModule : public IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override; 
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
	};
}
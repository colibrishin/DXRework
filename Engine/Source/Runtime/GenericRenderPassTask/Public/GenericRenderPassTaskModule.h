#pragma once
#include <memory>

#include "Source/Runtime/Core/ModuleManager/Public/IModule.h"

#include "GenericRenderPassTaskModule.generated.h"

namespace Engine 
{
	ECLASS(module)
	struct ENGINE_GENERICRENDERPASSTASK_API GenericRenderPassTaskModule : public IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override; 
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
	};
}
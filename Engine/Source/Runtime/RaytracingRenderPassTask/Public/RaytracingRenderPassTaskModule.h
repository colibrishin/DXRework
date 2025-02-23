#pragma once
#include "CoreType.h"
#include "ModuleManager.h"
#include "RaytracingRenderPassTaskModule.generated.h"

namespace Engine 
{
	ECLASS(module)
	struct ENGINE_RAYTRACINGRENDERPASSTASK_API RaytracingRenderPassTaskModule : public IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
	};
}
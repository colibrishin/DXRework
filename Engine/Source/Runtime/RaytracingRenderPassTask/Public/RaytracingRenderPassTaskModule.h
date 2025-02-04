#pragma once
#include <memory>

#include "Source/Runtime/Core/ModuleManager/Public/IModule.h"

#include "RaytracingRenderPassTaskModule.generated.h"

namespace Engine 
{
	ECLASS(module)
	struct ENGINE_RAYTRACINGRENDERPASSTASK_API RaytracingRenderPassTaskModule : public IModule
	{
		GENERATE_BODY
		void Initialize() override;
		void Shutdown() override;
		bool DynamicLoadable() override;
	};
}
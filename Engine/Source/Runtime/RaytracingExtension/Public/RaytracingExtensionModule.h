#pragma once
#if CFG_RAYTRACING
#include "IModule.h"
#include "ModuleManager.h"
#include "RaytracingExtensionModule.generated.h"

namespace Engine 
{
	ECLASS(module)
	struct ENGINE_RAYTRACINGEXTENSION_API RaytracingExtensionModule : public IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
		ELoadPhase GetLoadPhase() const override { return ELoadPhase::Graphic; }
	};
}
#endif
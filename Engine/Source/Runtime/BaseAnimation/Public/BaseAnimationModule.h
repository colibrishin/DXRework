#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "BaseAnimationModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct ENGINE_BASEANIMATION_API BaseAnimationModule : public Engine::IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
		ELoadPhase GetLoadPhase() const override { return ELoadPhase::Graphic; }
	};
}
#pragma once
#include "ModuleManager/Public/IModule.h"

#include "AnimatorModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct AnimatorModule : public IModule
	{
		GENERATE_BODY
		bool             InitializeImpl() override;
		bool             ShutdownImpl() override;
		bool             DynamicLoadable() override;
	};
}

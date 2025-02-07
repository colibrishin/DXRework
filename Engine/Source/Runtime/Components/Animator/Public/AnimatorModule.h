#pragma once
#include "ModuleManager/Public/IModule.h"

#include "AnimatorModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct AnimatorModule : public IModule
	{
		GENERATE_BODY
		void             Initialize() override;
		void             Shutdown() override;
		bool             DynamicLoadable() override;
	};
}

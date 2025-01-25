#pragma once
#include "ModuleManager/Public/IModule.h"
#include "ShapeModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct ShapeModule : IModule
	{
		GENERATE_BODY
		void             Initialize() override;
		void             Shutdown() override;
		bool             DynamicLoadable() override;
	};
}

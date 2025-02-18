#pragma once
#include "ModuleManager.h"
#include "CoreType.h"

#include "MaterialModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct ENGINE_MATERIAL_API MaterialModule : IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
	};
}
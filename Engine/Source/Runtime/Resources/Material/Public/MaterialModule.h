#pragma once
#include "CoreType.h"
#include "ModuleManager/Public/ModuleManager.h"

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
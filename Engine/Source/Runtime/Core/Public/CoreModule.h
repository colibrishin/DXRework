#pragma once
#include "IModule.h"
#include "Debugger.h"
#include "ModuleManager.h"
#include "ResourceManager.h"
#include "SceneManager.h"

#include "CoreModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct ENGINE_CORE_API CoreModule : public IModule
	{
		GENERATE_BODY

		bool InitializeImpl() override; 
		bool ShutdownImpl() override;
		bool DynamicLoadable() override
		{
			return true;
		}
		ELoadPhase GetLoadPhase() const override { return ELoadPhase::Core; }
	};
}


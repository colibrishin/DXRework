#pragma once
#include "Debugger/Public/Debugger.h"
#include "ModuleManager.h"
#include "ResourceManager/Public/ResourceManager.h"
#include "SceneManager/Public/SceneManager.h"

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
	};
}


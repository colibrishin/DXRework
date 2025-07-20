#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "SoundManagerModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct ENGINE_SOUNDMANAGER_API SoundManagerModule : public IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
		const std::vector<std::string>& LoadAfter() const override;
	};
}
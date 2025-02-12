#pragma once

#include "ModuleManager/Public/IModule.h"

#include "SoundPlayerModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct ENGINE_SOUNDPLAYER_API SoundPlayerModule : public IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
	};
}
#pragma once

#include "IModule.h"

#include "FontModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct ENGINE_FONT_API FontModule : public IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
	};
}
#pragma once
#include "IModule.h"

#include "Texture2DModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct Texture2DModule : public Engine::IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
        const std::vector<std::string> &LoadAfter() const override;
	};
}

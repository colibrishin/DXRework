#pragma once

#include "IModule.h"

#include "TextureModule.generated.h"

namespace Engine 
{
	ECLASS(module)
	struct ENGINE_TEXTURE_API TextureModule : public Engine::IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;

		const std::vector<std::string>& LoadAfter() const override;
	};
}
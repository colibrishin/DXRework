#pragma once
#include "IModule.h"

#include "AtlasAnimationTextureModule.generated.h"

namespace Engine 
{
	ECLASS(module)
	struct AtlasAnimationTextureModule : public IModule 
	{
		GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
	};
}
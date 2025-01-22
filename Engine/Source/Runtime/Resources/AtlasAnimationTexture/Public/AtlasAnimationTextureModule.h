#pragma once
#include "ModuleManager/Public/IModule.h"

#include "AtlasAnimationTextureModule.generated.h"

namespace Engine 
{
	ECLASS(module)
	struct AtlasAnimationTextureModule : public IModule 
	{
		GENERATE_BODY
		void Initialize() override;
		void Shutdown() override;
		bool DynamicLoadable() override;
	};
}
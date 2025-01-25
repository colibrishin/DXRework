#pragma once
#include "ModuleManager/Public/IModule.h"

#include "Texture2DModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct Texture2DModule : public Engine::IModule
	{
		INLINE_COMPILE_TIME_TYPENAME(Texture2DModule)
		void             Initialize() override;
		void             Shutdown() override;
		bool             DynamicLoadable() override;
	};
}

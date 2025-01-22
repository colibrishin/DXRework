#pragma once
#include "../Misc.h"

namespace Engine
{
	struct CORE_API IModule
	{
		virtual ~IModule() = default;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual bool DynamicLoadable() = 0;
	};
}
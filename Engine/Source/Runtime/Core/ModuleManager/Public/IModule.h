#pragma once
#include "../Misc.h"

#include "Serialization.hpp"
#include "IModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct ENGINE_CORE_API IModule
	{
		GENERATE_BODY
		virtual ~IModule() = default;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual bool DynamicLoadable() = 0;
	};
}
#pragma once
#include "CoreType.h"

#include "Serialization.hpp"
#include "IModule.generated.h"

namespace Engine
{
	ECLASS(abstract)
	struct ENGINE_CORE_API IModule
	{
		GENERATE_BODY
		virtual ~IModule() = default;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual bool DynamicLoadable() = 0;
		virtual const std::vector<std::string>& GetDependencies() const = 0;
	};
}
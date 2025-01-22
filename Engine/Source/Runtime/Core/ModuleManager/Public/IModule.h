#pragma once
#include "../Misc.h"

namespace Engine
{
	struct ENGINE_CORE_API IModule
	{
		virtual ~IModule() = default;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual bool DynamicLoadable() = 0;

		virtual std::string_view GetTypeName() const = 0;
		virtual std::string_view GetPrettyTypeName() const = 0;
	};
}
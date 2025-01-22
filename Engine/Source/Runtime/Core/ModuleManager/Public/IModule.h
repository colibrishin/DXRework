#pragma once
#include "../Misc.h"

namespace Engine
{
	struct IModule;
}

POLYMORPHIC_TYPE_MAP(Engine::IModule, void)

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
		virtual HashType GetTypeHash() const = 0;
		virtual bool IsBaseOf(HashType hash) const = 0;
	};
}
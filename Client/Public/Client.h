#pragma once
#include "Source/Runtime/Core/ModuleManager/Public/IModule.h"

#include "Client.generated.h"

ECLASS(module)
struct ClientModule : Engine::IModule
{
	GENERATE_BODY

	void Initialize() override;
	void Shutdown() override;
	bool DynamicLoadable() override;
};
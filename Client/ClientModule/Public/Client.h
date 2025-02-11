#pragma once
#include "Source/Runtime/Core/ModuleManager/Public/IClientModule.h"

#include "Client.generated.h"

ECLASS(clientModule)
struct ENGINE_CLIENT_API ClientModule : Engine::IClientModule
{
	GENERATE_BODY
	bool InitializeImpl() override;
	bool ShutdownImpl() override;
	bool DynamicLoadable() override;
};
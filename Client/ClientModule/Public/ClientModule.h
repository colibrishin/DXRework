#pragma once
#include "IClientModule.h"

#include "ClientModule.generated.h"

ECLASS(clientModule)
struct ENGINE_CLIENT_API ClientModule : Engine::IClientModule
{
	GENERATE_BODY
	bool InitializeImpl() override;
	bool ShutdownImpl() override;
	bool DynamicLoadable() override;
};
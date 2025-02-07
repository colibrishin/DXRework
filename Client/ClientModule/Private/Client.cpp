#include "ClientModule/Public/Client.h"
#include "Client.generated.h"

#include "ModuleManager/Public/ModuleManager.h"

#include "Script/Public/Script.h"

MODULE_IMPL(ClientModule, Client)

void ClientModule::Initialize()
{
    Engine::IClientModule::Initialize();
}

void ClientModule::Shutdown()
{
    Engine::IClientModule::Shutdown();
}

bool ClientModule::DynamicLoadable()
{
    return true;
}

#include "Client.h"
#include "Client.generated.h"

#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(ClientModule, Client)

void ClientModule::Initialize()
{
}

void ClientModule::Shutdown()
{
}

bool ClientModule::DynamicLoadable()
{
    return true;
}

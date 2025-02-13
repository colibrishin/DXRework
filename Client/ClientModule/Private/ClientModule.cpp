#include "ClientModule/Public/ClientModule.h"
#include "ClientModule.generated.h"

#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(ClientModule, Client)

bool ClientModule::InitializeImpl()
{
    return Engine::IClientModule::InitializeImpl();
}

bool ClientModule::ShutdownImpl()
{
    return Engine::IClientModule::ShutdownImpl();
}

bool ClientModule::DynamicLoadable()
{
    return true;
}

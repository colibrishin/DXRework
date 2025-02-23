#include "EngineEntryPointModule.h"
#include "EngineEntryPointModule.generated.h"

bool Engine::EngineEntryPointModule::InitializeImpl()
{
    return true;
}

bool Engine::EngineEntryPointModule::ShutdownImpl()
{
    return true;
}

bool Engine::EngineEntryPointModule::DynamicLoadable()
{
    return true;
}

const std::vector<std::string> & Engine::EngineEntryPointModule::LoadAfter() const
{
    static const std::vector<std::string> load_after = { "ModuleManager" };
    return load_after;
}

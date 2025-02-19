#include "Texture1DModule.h"
#include "Texture1DModule.generated.h"



MODULE_IMPL(Engine::Texture1DModule, Texture1D)

bool Engine::Texture1DModule::InitializeImpl()
{
    return true;
}

bool Engine::Texture1DModule::ShutdownImpl()
{
    return true;
}

bool Engine::Texture1DModule::DynamicLoadable()
{
    return true;
}

const std::vector<std::string> &Engine::Texture1DModule::LoadAfter() const
{
    static std::vector<std::string> load_after = { "RenderPipeline", "Texture" };
    return load_after;
}

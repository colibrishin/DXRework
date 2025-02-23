#include "Texture3DModule.h"
#include "Texture3DModule.generated.h"

MODULE_IMPL( Engine::Texture3DModule, Texture3D )

bool Engine::Texture3DModule::InitializeImpl()
{
    return true;
}

bool Engine::Texture3DModule::ShutdownImpl()
{
    return true;
}

const std::vector<std::string> &Engine::Texture3DModule::LoadAfter() const
{
    static std::vector<std::string> load_after = { "RenderPipeline", "Texture" };
    return load_after;
}

bool Engine::Texture3DModule::DynamicLoadable()
{
    return true;
}

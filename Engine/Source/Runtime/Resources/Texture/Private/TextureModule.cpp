#include "TextureModule.h"
#include "TextureModule.generated.h"

#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::TextureModule, Texture)

bool Engine::TextureModule::InitializeImpl()
{
    return true;
}

bool Engine::TextureModule::ShutdownImpl()
{
    return true;
}

bool Engine::TextureModule::DynamicLoadable()
{
    return false;
}

const std::vector<std::string>& Engine::TextureModule::LoadAfter() const
{
    static std::vector<std::string> load_after = { "RenderPipeline" };
    return load_after;
}

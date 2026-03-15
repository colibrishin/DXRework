#include "ShadowTextureModule.h"


MODULE_IMPL( Engine::ShadowTextureModule, ShadowTexture )

bool Engine::ShadowTextureModule::InitializeImpl()
{
    return true;
}

bool Engine::ShadowTextureModule::ShutdownImpl()
{
    return true;
}

bool Engine::ShadowTextureModule::DynamicLoadable()
{
    return true;
}

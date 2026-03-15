#include "AnimationTextureModule.h"

MODULE_IMPL( Engine::AnimationTextureModule, AnimationTexture )

bool Engine::AnimationTextureModule::InitializeImpl()
{
    return true;
}

bool Engine::AnimationTextureModule::ShutdownImpl()
{
    return true;
}

bool Engine::AnimationTextureModule::DynamicLoadable()
{
    return true;
}

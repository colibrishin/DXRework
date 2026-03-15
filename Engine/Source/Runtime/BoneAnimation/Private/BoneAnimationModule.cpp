#include "BoneAnimationModule.h"

MODULE_IMPL( Engine::BoneAnimationModule, BoneAnimation )

bool Engine::BoneAnimationModule::InitializeImpl()
{
    return false;
}

bool Engine::BoneAnimationModule::ShutdownImpl()
{
    return false;
}

bool Engine::BoneAnimationModule::DynamicLoadable()
{
    return false;
}

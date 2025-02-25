#include "BoneModule.h"
#include "BoneModule.generated.h"

MODULE_IMPL( Engine::BoneModule, Bone )

bool Engine::BoneModule::InitializeImpl()
{
    return true;
}

bool Engine::BoneModule::ShutdownImpl()
{
    return true;
}

bool Engine::BoneModule::DynamicLoadable()
{
    return true;
}

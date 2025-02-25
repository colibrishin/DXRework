#include "AtlasAnimationModule.h"
#include "AtlasAnimationModule.generated.h"

MODULE_IMPL( Engine::AtlasAnimtionModule, AtlasAnimation )

bool Engine::AtlasAnimtionModule::InitializeImpl()
{
    return false;
}

bool Engine::AtlasAnimtionModule::ShutdownImpl()
{
    return false;
}

bool Engine::AtlasAnimtionModule::DynamicLoadable()
{
    return false;
}

#include "BaseAnimationModule.h"
#include "BaseAnimationModule.generated.h"
#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::BaseAnimationModule, BaseAnimation)

bool Engine::BaseAnimationModule::InitializeImpl()
{
    return true;
}

bool Engine::BaseAnimationModule::ShutdownImpl()
{
    return true;
}

bool Engine::BaseAnimationModule::DynamicLoadable()
{
    return true;
}
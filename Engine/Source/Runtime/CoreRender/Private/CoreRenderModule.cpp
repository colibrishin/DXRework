#include "CoreRenderModule.h"
#include "CoreRenderModule.generated.h"

MODULE_IMPL( Engine::CoreRenderModule, CoreRender )

bool Engine::CoreRenderModule::InitializeImpl()
{
    return true;
}

bool Engine::CoreRenderModule::ShutdownImpl()
{
    return true;
}

bool Engine::CoreRenderModule::DynamicLoadable()
{
    return true;
}

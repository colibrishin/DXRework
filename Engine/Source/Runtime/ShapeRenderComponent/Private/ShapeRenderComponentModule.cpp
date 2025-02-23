#include "ShapeRenderComponentModule.h"
#include "ShapeRenderComponentModule.generated.h"

MODULE_IMPL(Engine::ShapeRenderComponentModule, ShapeRenderComponent)

bool Engine::ShapeRenderComponentModule::InitializeImpl()
{
    return true;
}

bool Engine::ShapeRenderComponentModule::ShutdownImpl()
{
    return true;
}

bool Engine::ShapeRenderComponentModule::DynamicLoadable()
{
    return true;
}

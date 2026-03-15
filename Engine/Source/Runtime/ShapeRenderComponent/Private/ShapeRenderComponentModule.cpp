#include "ShapeRenderComponentModule.h"

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

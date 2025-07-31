#include "RendererModule.h"
#include "RendererModule.generated.h"

bool Engine::RendererModule::InitializeImpl()
{
    return true;
}

bool Engine::RendererModule::ShutdownImpl()
{
    return true;
}

bool Engine::RendererModule::DynamicLoadable()
{
    return true;
}

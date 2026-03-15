#include "RenderComponentModule.h"

MODULE_IMPL( Engine::RenderComponentModule, RenderComponent )

namespace Engine
{
    bool Engine::RenderComponentModule::InitializeImpl()
    {
        return true;
    }
    bool Engine::RenderComponentModule::ShutdownImpl()
    {
        return true;
    }
    bool Engine::RenderComponentModule::DynamicLoadable()
    {
        return true;
    }
} // namespace Engine

#include "TextRendererModule.h"
#include "TextRendererModule.generated.h"

#include "TextRenderer.h"

#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::TextRendererModule, TextRenderer)

namespace Engine
{
    bool Engine::TextRendererModule::InitializeImpl()
    {
        Engine::ComponentFactory::Register<Components::TextRenderer>();
        return true;
    }
    bool Engine::TextRendererModule::ShutdownImpl()
    {
        Engine::ComponentFactory::Unregister<Components::TextRenderer>();
        return true;
    }
    bool Engine::TextRendererModule::DynamicLoadable()
    {
        return true;
    }
}
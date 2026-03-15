#include "TextRendererModule.h"

#include "TextRenderer.h"

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
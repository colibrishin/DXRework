#include "ModelRendererModule.h"
#include "ModelRendererModule.generated.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "ModelRenderer.h"
#include "ObjectBase/Public/ObjectBase.h"
#include "Component/Public/Component.h"

MODULE_IMPL(Engine::ModelRendererModule, ModelRenderer)

bool Engine::ModelRendererModule::InitializeImpl()
{
    Engine::ComponentFactory::Register<Engine::Components::ModelRenderer>();
    return true;
}

bool Engine::ModelRendererModule::ShutdownImpl()
{
    Engine::ComponentFactory::Unregister<Engine::Components::ModelRenderer>();
    return true;
}

bool Engine::ModelRendererModule::DynamicLoadable()
{
    return true;
}

#include "ModelRendererModule.h"
#include "ModelRendererModule.generated.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "ModelRenderer.h"
#include "ObjectBase/Public/ObjectBase.h"

MODULE_IMPL(Engine::ModelRendererModule, ModelRenderer)

void Engine::ModelRendererModule::Initialize()
{
    Abstracts::ObjectBase::RegisterComponentFactory("ModelRenderer", [](const Weak<Abstracts::ObjectBase>& owner)
    {
        if (const Strong<Abstracts::ObjectBase>& locked = owner.lock())
        {
            locked->AddComponent<Components::ModelRenderer>();
        }
    });
}

void Engine::ModelRendererModule::Shutdown()
{
    Abstracts::ObjectBase::UnregisterComponentFactory("ModelRenderer");
}

bool Engine::ModelRendererModule::DynamicLoadable()
{
    return true;
}

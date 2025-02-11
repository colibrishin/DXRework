#include "ModelRendererModule.h"
#include "ModelRendererModule.generated.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "ModelRenderer.h"
#include "ObjectBase/Public/ObjectBase.h"

MODULE_IMPL(Engine::ModelRendererModule, ModelRenderer)

bool Engine::ModelRendererModule::InitializeImpl()
{
    Abstracts::ObjectBase::RegisterComponentFactory("ModelRenderer", [](const Weak<Abstracts::ObjectBase>& owner)
    {
        if (const Strong<Abstracts::ObjectBase>& locked = owner.lock())
        {
            locked->AddComponent<Components::ModelRenderer>();
        }
    });

    return true;
}

bool Engine::ModelRendererModule::ShutdownImpl()
{
    Abstracts::ObjectBase::UnregisterComponentFactory("ModelRenderer");

    return true;
}

bool Engine::ModelRendererModule::DynamicLoadable()
{
    return true;
}

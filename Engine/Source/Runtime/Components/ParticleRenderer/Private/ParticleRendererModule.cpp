#include "ParticleRendererModule.h"
#include "ParticleRendererModule.generated.h"

#include "ComputeShader.h"
#include "ModuleManager/Public/ModuleManager.h"

#include "ParticleRenderer.h"

#include "ObjectBase/Public/ObjectBase.h"

MODULE_IMPL(Engine::ParticleRendererModule, ParticleRenderer)

bool Engine::ParticleRendererModule::InitializeImpl()
{
    Abstracts::ObjectBase::RegisterComponentFactory("ParticleRenderer", [](const Weak<Abstracts::ObjectBase>& owner)
    {
        if (const Strong<Abstracts::ObjectBase>& locked = owner.lock())
        {
            locked->AddComponent<Components::ParticleRenderer>();
        }
    });

    return true;
}

bool Engine::ParticleRendererModule::ShutdownImpl()
{
    Abstracts::ObjectBase::UnregisterComponentFactory("ParticleRenderer");

    return true;
}

bool Engine::ParticleRendererModule::DynamicLoadable()
{
    return true;
}

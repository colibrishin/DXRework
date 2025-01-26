#include "ParticleRendererModule.h"

#include "ComputeShader.h"
#include "ParticleRendererModule.generated.h"
#include "ModuleManager/Public/ModuleManager.h"

#include "ParticleRenderer.h"
#include "SimpleParticleComputeShader.h"

#include "ObjectBase/Public/ObjectBase.h"

MODULE_IMPL(Engine::ParticleRendererModule, ParticleRenderer)

void Engine::ParticleRendererModule::Initialize()
{
    Abstracts::ObjectBase::RegisterComponentFactory("ParticleRenderer", [](const Weak<Abstracts::ObjectBase>& owner)
    {
        if (const Strong<Abstracts::ObjectBase>& locked = owner.lock())
        {
            locked->AddComponent<Components::ParticleRenderer>();
        }
    });

    Resources::SimpleParticleComputeShader::Create("SimpleParticleComputeShader");
}

void Engine::ParticleRendererModule::Shutdown()
{
    Abstracts::ObjectBase::UnregisterComponentFactory("ParticleRenderer");
}

bool Engine::ParticleRendererModule::DynamicLoadable()
{
    return true;
}

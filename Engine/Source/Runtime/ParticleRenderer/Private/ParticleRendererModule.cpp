#include "ParticleRendererModule.h"

#include "ComputeShader.h"
#include "ParticleRenderer.h"

#include "ObjectBase.h"

MODULE_IMPL(Engine::ParticleRendererModule, ParticleRenderer)

bool Engine::ParticleRendererModule::InitializeImpl()
{
    Engine::ComponentFactory::Register<Engine::Components::ParticleRenderer>();
    return true;
}

bool Engine::ParticleRendererModule::ShutdownImpl()
{
    Engine::ComponentFactory::Unregister<Engine::Components::ParticleRenderer>();
    return true;
}

bool Engine::ParticleRendererModule::DynamicLoadable()
{
    return true;
}

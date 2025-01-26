#include "ParticleRendererExtensionModule.h"
#include "ParticleRendererExtensionModule.generated.h"

#include "SimpleParticleComputeShader.h"

#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::ParticleRendererExtensionModule, ParticleRendererExtension)

void Engine::ParticleRendererExtensionModule::Initialize()
{
    Resources::SimpleParticleComputeShader::Create("SimpleParticleComputeShader");
}

void Engine::ParticleRendererExtensionModule::Shutdown()
{
}

bool Engine::ParticleRendererExtensionModule::DynamicLoadable()
{
    return true;
}

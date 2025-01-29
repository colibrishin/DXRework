#include "ParticleRendererRenderInstanceTaskModule.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "ParticleRendererRenderTask.h"
#include "Renderer.h"

MODULE_IMPL(Engine::ParticleRendererRenderInstanceTaskModule, ParticleRendererRenderInstanceTask)

namespace Engine
{
    void ParticleRendererRenderInstanceTaskModule::Initialize()
    {
        Managers::Renderer::GetInstance().RegisterRenderInstance(
            L"ParticleRendererRenderInstanceTask",
            new ParticleRendererRenderInstanceTask() );
    }

    void ParticleRendererRenderInstanceTaskModule::Shutdown()
    {
        Managers::Renderer::GetInstance().UnregisterRenderInstance(L"ParticleRendererRenderInstanceTask");
    }

    bool ParticleRendererRenderInstanceTaskModule::DynamicLoadable() { return true; }
}

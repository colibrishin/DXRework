#include "ParticleRendererRenderInstanceTaskModule.h"
#include "ParticleRendererRenderInstanceTaskModule.generated.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "ParticleRendererRenderTask.h"
#include "Renderer.h"

MODULE_IMPL(Engine::ParticleRendererRenderInstanceTaskModule, ParticleRendererRenderInstanceTask)

namespace Engine
{
    bool ParticleRendererRenderInstanceTaskModule::InitializeImpl()
    {
        Managers::Renderer::GetInstance().RegisterRenderInstance(
            L"ParticleRendererRenderInstanceTask",
            new ParticleRendererRenderInstanceTask() );

        return true;
    }

    bool ParticleRendererRenderInstanceTaskModule::ShutdownImpl()
    {
        Managers::Renderer::GetInstance().UnregisterRenderInstance(L"ParticleRendererRenderInstanceTask");

        return true;
    }

    bool ParticleRendererRenderInstanceTaskModule::DynamicLoadable() { return true; }
}

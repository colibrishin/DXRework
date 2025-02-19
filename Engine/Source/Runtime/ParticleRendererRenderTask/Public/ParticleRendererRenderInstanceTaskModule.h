#pragma once
#include "ModuleManager.h"

#include "ParticleRendererRenderInstanceTaskModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_PARTICLERENDERERRENDERTASK_API ParticleRendererRenderInstanceTaskModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };

}

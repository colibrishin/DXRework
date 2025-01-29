#pragma once
#include "ModuleManager/Public/IModule.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_PARTICLERENDERERRENDERTASK_API ParticleRendererRenderInstanceTaskModule : public IModule
    {
        GENERATE_BODY
        void Initialize() override;
        void Shutdown() override;
        bool DynamicLoadable() override;
    };

}

#pragma once
#include "ParticleRendererModule.generated.h"

#include "ModuleManager/Public/IModule.h"

namespace Engine
{
    struct ENGINE_PARTICLERENDERER_API ParticleRendererModule : public IModule
    {
        GENERATE_BODY
        void             Initialize() override;
        void             Shutdown() override;
        bool             DynamicLoadable() override;
    };
}

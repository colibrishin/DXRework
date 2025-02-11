#pragma once
#include "ModuleManager/Public/IModule.h"

#include "ParticleRendererModule.generated.h"

namespace Engine
{
    ECLASS(module) 
    struct ENGINE_PARTICLERENDERER_API ParticleRendererModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override; 
        bool ShutdownImpl() override;
        bool             DynamicLoadable() override;
    };
}

#pragma once
#include "IModule.h"

#include "ParticleRendererExtensionModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_PARTICLERENDEREREXTENSION_API ParticleRendererExtensionModule : public Engine::IModule
    {
        GENERATE_BODY
    public:
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };   
}

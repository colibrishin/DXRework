#pragma once
#include "ModuleManager/Public/IModule.h"

#include "ParticleRendererExtensionModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_PARTICLERENDEREREXTENSION_API ParticleRendererExtensionModule : public Engine::IModule
    {
        GENERATE_BODY
    public:
        void Initialize() override;
        void Shutdown() override;
        bool DynamicLoadable() override;
    };   
}

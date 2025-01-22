#pragma once
#include "ModuleManager/Public/IModule.h"

#include "ShadowManagerModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_SHADOWMANAGER_API ShadowManagerModule : public Engine::IModule
    {
        GENERATE_BODY
        void Initialize() override;
        void Shutdown() override;
        bool DynamicLoadable() override;
    };
}

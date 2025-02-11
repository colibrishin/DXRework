#pragma once
#include "ModuleManager/Public/IModule.h"

#include "ShadowManagerModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_SHADOWMANAGER_API ShadowManagerModule : public Engine::IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
}

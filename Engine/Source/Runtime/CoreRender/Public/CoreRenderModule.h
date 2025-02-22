#pragma once
#include "CoreType.h"
#include "ModuleManager.h"

#include "CoreRenderModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_CORERENDER_API CoreRenderModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
}
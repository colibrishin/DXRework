#pragma once
#include "ModuleManager.h"
#include "BoostSocketWrapperModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_BOOSTSOCKETWRAPPER_API BoostSocketWrapperModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
}
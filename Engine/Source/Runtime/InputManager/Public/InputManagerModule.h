#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "InputManagerModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_INPUTMANAGER_API InputManagerModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        ELoadPhase GetLoadPhase() const override { return ELoadPhase::Internal; }
    };
}

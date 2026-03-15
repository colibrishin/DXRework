#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "DirectInputInterfaceModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_DIRECTINPUTINTERFACE_API DirectInputInterfaceModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        ELoadPhase GetLoadPhase() const override { return ELoadPhase::Internal; }
    };
}
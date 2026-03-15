#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "AnimatorModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct AnimatorModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        ELoadPhase GetLoadPhase() const override { return ELoadPhase::Internal; }
    };
} // namespace Engine

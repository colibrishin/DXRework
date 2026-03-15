#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "DeferredRenderPassTaskModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_DEFERREDRENDERPASSTASK_API DeferredRenderPassTaskModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        ELoadPhase GetLoadPhase() const override { return ELoadPhase::Graphic; }
    };
} // namespace Engine

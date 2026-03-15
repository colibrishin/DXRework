#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "ModelRendererModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ModelRendererModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        ELoadPhase GetLoadPhase() const override { return ELoadPhase::Graphic; }
    };
} // namespace Engine

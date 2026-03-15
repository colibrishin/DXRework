#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "ImGuiManagerModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_IMGUIMANAGER_API ImGuiManagerModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override; 
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        ELoadPhase GetLoadPhase() const override { return ELoadPhase::UI; }
    };
}
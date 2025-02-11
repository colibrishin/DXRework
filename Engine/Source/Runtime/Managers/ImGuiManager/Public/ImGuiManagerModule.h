#pragma once

#include "ModuleManager/Public/IModule.h"

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
        const std::vector<std::string>& LoadAfter() const override;
    };
}
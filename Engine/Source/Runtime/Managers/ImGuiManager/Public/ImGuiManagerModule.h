#pragma once

#include "ModuleManager/Public/IModule.h"

#include "ImGuiManagerModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_IMGUIMANAGER_API ImGuiManagerModule : public IModule
    {
        GENERATE_BODY
        void Initialize() override;
        void Shutdown() override;
        bool DynamicLoadable() override;
    };
}
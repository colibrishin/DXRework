#pragma once
#include "ModuleManager.h"

#include "CoreUIModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_COREUI_API CoreUIModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
} // namespace Engine
#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "ShapeRenderComponentModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_SHAPERENDERCOMPONENT_API ShapeRenderComponentModule : IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
} // namespace Engine
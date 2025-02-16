#pragma once
#include "CoreType.h"
#include "IModule.h"

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
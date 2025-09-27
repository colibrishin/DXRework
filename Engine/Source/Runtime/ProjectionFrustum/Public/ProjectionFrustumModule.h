#pragma once
#include "CoreType.h"
#include "IModule.h"
#include "ModuleManager.h"

#include "ProjectionFrustumModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_PROJECTIONFRUSTUM_API ProjectionFrustumModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
}

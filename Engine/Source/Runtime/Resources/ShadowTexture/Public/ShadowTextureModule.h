#pragma once
#include "CoreType.h"
#include "IModule.h"

#include "ShadowTextureModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_SHADOWTEXTURE_API ShadowTextureModule : IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
} // namespace Engine
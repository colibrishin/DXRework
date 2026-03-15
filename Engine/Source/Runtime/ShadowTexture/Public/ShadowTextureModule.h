#pragma once
#include "IModule.h"
#include "ModuleManager.h"

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
        ELoadPhase GetLoadPhase() const override { return ELoadPhase::Graphic; }
    };
} // namespace Engine
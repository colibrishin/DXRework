#pragma once
#include "IModule.h"
#include "ModuleManager.h"
#include "AnimationTextureModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_ANIMATIONTEXTURE_API AnimationTextureModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
}

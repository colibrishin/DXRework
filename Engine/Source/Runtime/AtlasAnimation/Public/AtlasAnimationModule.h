#pragma once
#include "ModuleManager.h"
#include "AtlasAnimationModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_ATLASANIMATION_API AtlasAnimtionModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
}
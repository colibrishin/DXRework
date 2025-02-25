#pragma once

#include "ModuleManager.h"
#include "BoneAnimationModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_BONEANIMATION_API BoneAnimationModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
}
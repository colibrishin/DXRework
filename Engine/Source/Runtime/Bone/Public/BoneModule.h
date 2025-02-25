#pragma once
#include "ModuleManager.h"
#include "BoneModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_BONE_API BoneModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
}

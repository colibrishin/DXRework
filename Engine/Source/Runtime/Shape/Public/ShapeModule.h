#pragma once
#include "IModule.h"
#include "ModuleManager.h"
#include "ShapeModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ShapeModule : IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
} // namespace Engine


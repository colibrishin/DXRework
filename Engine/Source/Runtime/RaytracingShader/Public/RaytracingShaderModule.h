#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "RaytracingShaderModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_RAYTRACINGSHADER_API RaytracingShaderModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        ELoadPhase GetLoadPhase() const override { return ELoadPhase::Graphic; }
    };
}

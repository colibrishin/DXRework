#pragma once
#include "ModuleManager/Public/IModule.h"

namespace Engine
{
    struct ENGINE_RAYTRACINGSHADER_API RaytracingShaderModule : public IModule
    {
        GENERATE_BODY
        void Initialize() override;
        void Shutdown() override;
        bool DynamicLoadable() override;
    };
}

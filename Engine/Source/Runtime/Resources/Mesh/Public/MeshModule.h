#pragma once
#include "ModuleManager/Public/IModule.h"

#include "MeshModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_MESH_API MeshModule : public Engine::IModule
    {
        GENERATE_BODY
        void Initialize() override;
        void Shutdown() override;
        bool DynamicLoadable() override;
    };
}
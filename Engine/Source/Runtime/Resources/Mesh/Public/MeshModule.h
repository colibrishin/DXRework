#pragma once
#include "IModule.h"

#include "MeshModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_MESH_API MeshModule : public Engine::IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        const std::vector<std::string>& LoadAfter() const override;
    };
}
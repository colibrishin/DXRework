#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "MeshModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_MESH_API MeshModule : public Engine::IModule
    {
        GENERATE_BODY
        bool                            InitializeImpl() override;
        bool                            ShutdownImpl() override;
        bool                            DynamicLoadable() override;
        ELoadPhase GetLoadPhase() const override { return ELoadPhase::Graphic; }
    };
} // namespace Engine

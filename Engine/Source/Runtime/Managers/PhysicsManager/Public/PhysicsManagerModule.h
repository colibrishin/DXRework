#pragma once
#include "IModule.h"

#include "PhysicsManagerModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct PhysicsManagerModule : public Engine::IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override; 
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
}
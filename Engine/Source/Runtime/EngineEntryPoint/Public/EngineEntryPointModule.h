#pragma once
#include "CoreType.h"
#include "ModuleManager.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_ENGINEENTRYPOINT_API EngineEntryPointModule : public Engine::IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        const std::vector<std::string> &LoadAfter() const override;
    };
}

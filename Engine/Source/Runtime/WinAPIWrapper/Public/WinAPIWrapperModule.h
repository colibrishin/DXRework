#pragma once

#include "ModuleManager.h"

#include "WinAPIWrapperModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_WINAPIWRAPPER_API WinAPIWrapperModule : public Engine::IModule
    {
    public:
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
} // namespace WinAPI

#pragma once
#include "CoreType.h"
#include "ModuleManager.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_INPUTMANAGER_API InputManagerModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
}

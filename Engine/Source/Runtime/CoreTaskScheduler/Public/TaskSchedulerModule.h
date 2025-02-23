#pragma once
#include "CoreType.h"
#include "ModuleManager.h"

#include "TaskSchedulerModule.generated.h"

namespace Engine
{
    ECLASS( module ) 
    struct ENGINE_CORETASKSCHEDULER_API TaskSchedulerModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;   
    };
}

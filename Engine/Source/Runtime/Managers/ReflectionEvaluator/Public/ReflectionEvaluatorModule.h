#pragma once
#include "IModule.h"

#include "ReflectionEvaluatorModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ReflectionEvaluatorModule : public Engine::IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };   
}

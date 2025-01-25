#pragma once
#include "ModuleManager/Public/IModule.h"

namespace Engine
{
    ECLASS(module)
    struct ReflectionEvaluatorModule : public Engine::IModule
    {
        GENERATE_BODY
        void Initialize() override;
        void Shutdown() override;
        bool DynamicLoadable() override;
    };   
}

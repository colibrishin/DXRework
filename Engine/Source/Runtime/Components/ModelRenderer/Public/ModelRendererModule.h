#pragma once
#include "ModelRendererModule.generated.h"
#include "ModuleManager/Public/IModule.h"

namespace Engine
{
    struct ModelRendererModule : public IModule
    {
        GENERATE_BODY
        void             Initialize() override;
        void             Shutdown() override;
        bool             DynamicLoadable() override;
    };
}

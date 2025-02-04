#pragma once
#include "ModuleManager/Public/IModule.h"

#include "CameraManagerModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_CAMERAMANAGER_API CameraManagerModule : public IModule
    {
        GENERATE_BODY
        void Initialize() override;
        void Shutdown() override;
        bool DynamicLoadable() override;
    };
}

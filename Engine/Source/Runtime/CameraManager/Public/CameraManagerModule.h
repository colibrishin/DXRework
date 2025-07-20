#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "CameraManagerModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_CAMERAMANAGER_API CameraManagerModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        const std::vector<std::string>& LoadAfter() const override;
    };
}

#pragma once

#include "IModule.h"

#include "Texture3DModule.generated.h"

namespace Engine
{
    ECLASS( module )
    class ENGINE_TEXTURE3D_API Texture3DModule : public Engine::IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        const std::vector<std::string> &LoadAfter() const override;
    };
}
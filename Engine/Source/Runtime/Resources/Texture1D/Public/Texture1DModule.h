#pragma once
#include "IModule.h"

#include "Texture1DModule.generated.h"

namespace Engine
{
    ECLASS( module )
    class ENGINE_TEXTURE1D_API Texture1DModule : public Engine::IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        const std::vector<std::string>& LoadAfter() const override;
    };
}
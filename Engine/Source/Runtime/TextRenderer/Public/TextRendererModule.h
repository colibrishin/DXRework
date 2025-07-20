#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "TextRendererModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_TEXTRENDERER_API TextRendererModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
} // namespace Engine

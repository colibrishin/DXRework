#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "RendererModule.generated.h"

namespace Engine
{
    ECLASS(module)
	struct ENGINE_RENDERPIPELINE_API RendererModule : public IModule
	{
        GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
	};
}
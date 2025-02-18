#pragma once
#include "ModuleManager.h"

#include "RenderPipelineModule.generated.h"

namespace Engine
{
    ECLASS(module)
	struct ENGINE_RENDERPIPELINE_API RenderPipelineModule : public IModule
	{
        GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
		const std::vector<std::string>& LoadAfter() const override;
	};
}
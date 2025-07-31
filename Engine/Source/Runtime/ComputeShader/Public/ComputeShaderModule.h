#pragma once
#include "IModule.h"
#include "ModuleManager.h"

#include "ComputeShaderModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct ENGINE_COMPUTESHADER_API ComputeShaderModule : public Engine::IModule
	{
		GENERATE_BODY
		
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
        bool DynamicLoadable() override;
        const std::vector<std::string>& LoadAfter() const override;
	};
} // namespace Engine::Resources

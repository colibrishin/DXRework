#pragma once

#include "ModuleManager/Public/IModule.h"
#include "ComputeShaderModule.generated.h"

namespace Engine::Resources
{
	ECLASS(module)
	struct ENGINE_COMPUTESHADER_API ComputeShaderModule : public Engine::IModule
	{
		GENERATE_BODY
		
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;

	};
} // namespace Engine::Resources

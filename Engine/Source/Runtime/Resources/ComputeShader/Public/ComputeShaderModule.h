#pragma once
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

	};
} // namespace Engine::Resources

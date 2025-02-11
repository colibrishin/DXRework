#pragma once

#include "ModuleManager/Public/IModule.h"

#include "ShaderModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct ShaderModule : public IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;

		void StockShaderPrecompile();
		const std::vector<std::string>& LoadAfter() const;
	};
}
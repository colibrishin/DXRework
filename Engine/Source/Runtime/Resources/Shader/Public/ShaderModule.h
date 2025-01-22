#pragma once

#include "ModuleManager/Public/IModule.h"

#include "ShaderModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct ShaderModule : public IModule
	{
		GENERATE_BODY
		void             Initialize() override;
		void             Shutdown() override;
		bool             DynamicLoadable() override;

		void StockShaderPrecompile();
	};
}
#pragma once
#include "IModule.h"

#include "IClientModule.generated.h"

namespace Engine 
{
	ECLASS(abstract)
	struct ENGINE_CORETYPE_API IClientModule : public IModule
	{
		GENERATE_BODY

		virtual void GeneratedInitialize();
		virtual void GeneratedShutdown();

		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		
		const std::vector<std::string>& LoadAfter() const override;
	};
}
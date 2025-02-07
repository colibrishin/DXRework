#pragma once
#include "IModule.h"

#include "IClientModule.generated.h"

namespace Engine 
{
	ECLASS(abstract)
	struct ENGINE_CORE_API IClientModule : public IModule
	{
		GENERATE_BODY

		virtual void GeneratedInitialize();
		virtual void GeneratedShutdown();

		void Initialize() override;
		void Shutdown() override;
	};
}
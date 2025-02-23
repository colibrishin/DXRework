#pragma once
#include "ModuleManager.h"

namespace Engine 
{
	struct ENGINE_CORE_API IClientModule : public Engine::IModule
	{
		virtual void GeneratedInitialize();
		virtual void GeneratedShutdown();

		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		
		const std::vector<std::string>& LoadAfter() const override;
	};
}
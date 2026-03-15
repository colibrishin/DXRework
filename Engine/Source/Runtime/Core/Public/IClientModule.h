#pragma once
#include "IModule.h"
#include "ModuleManager.h"

namespace Engine 
{
	struct ENGINE_CORE_API IClientModule : public Engine::IModule
	{
		virtual void GeneratedInitialize();
		virtual void GeneratedShutdown();

		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		
		ELoadPhase GetLoadPhase() const override { return ELoadPhase::Application; }
	};
}
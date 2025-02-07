#include "ModuleManager/Public/IClientModule.h"
#include "IClientModule.generated.h"

namespace Engine 
{
	inline void IClientModule::GeneratedInitialize() {}

	inline void IClientModule::GeneratedShutdown() {}

	inline void IClientModule::Initialize()
	{
		GeneratedInitialize();
	}

	inline void IClientModule::Shutdown()
	{
		GeneratedShutdown();
	}
}
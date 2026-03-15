#include "IClientModule.h"

namespace Engine 
{
	inline void IClientModule::GeneratedInitialize() {}

	inline void IClientModule::GeneratedShutdown() {}

	inline bool IClientModule::InitializeImpl()
	{
		GeneratedInitialize();

		return true;
	}

	inline bool IClientModule::ShutdownImpl()
	{
		GeneratedShutdown();

		return true;
	}

}
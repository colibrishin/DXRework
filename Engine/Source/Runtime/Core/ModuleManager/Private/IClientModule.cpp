#include "ModuleManager/Public/IClientModule.h"
#include "IClientModule.generated.h"

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

	const std::vector<std::string>& IClientModule::LoadAfter() const
	{
		return GetDependencies();
	}
}
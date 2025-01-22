#include "../Misc.h"
#include "ModuleManager/Public/ModuleManager.h"

#include "MaterialModule.generated.h"

namespace Engine
{
	ECLASS(module)
	struct MaterialModule : IModule
	{
		GENERATE_BODY
		void Initialize() override;
		void Shutdown() override;
		bool DynamicLoadable() override;
	};
}
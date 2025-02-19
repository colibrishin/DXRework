#include "ModuleManager.h"

#include "CoreTypeModule.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ENGINE_CORETYPE_API CoreTypeModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
}
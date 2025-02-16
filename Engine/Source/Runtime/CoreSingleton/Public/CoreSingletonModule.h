#include "IModule.h"

#include "CoreSingletonModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_CORESINGLETON_API CoreSingletonModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
} // namespace Engine
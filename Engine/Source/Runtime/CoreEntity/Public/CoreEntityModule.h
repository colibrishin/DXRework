#include "IModule.h"

#include "CoreEntityModule.generated.h"

namespace Engine
{
    ECLASS( module )
    struct ENGINE_COREENTITY_API CoreEntityModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
        bool DynamicLoadable() override;
    };
} // namespace Engine
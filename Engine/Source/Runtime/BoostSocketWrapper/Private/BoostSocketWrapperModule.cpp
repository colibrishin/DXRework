#pragma once
#include "BoostSocketWrapper.h"
#include "BoostSocketWrapperModule.h"
#include "BoostSocketWrapperModule.generated.h"

namespace Engine
{
    bool Engine::BoostSocketWrapperModule::InitializeImpl()
    {
        s_nia.SetInterface<BoostSocketWrapper>();
        return true;
    }
    bool BoostSocketWrapperModule::ShutdownImpl()
    {
        s_nia.Shutdown();
        return true;
    }
    bool BoostSocketWrapperModule::DynamicLoadable()
    {
        return true;
    }
} // namespace Engine

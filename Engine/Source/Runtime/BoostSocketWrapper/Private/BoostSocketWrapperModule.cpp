#pragma once
#include "BoostSocketWrapper.h"
#include "BoostSocketWrapperModule.h"
#include "BoostSocketWrapperModule.generated.h"

MODULE_IMPL(Engine::BoostSocketWrapperModule, BoostSocketWrapper);

namespace Engine
{
    bool Engine::BoostSocketWrapperModule::InitializeImpl()
    {
        g_network_accessor.SetInterface<BoostSocketWrapper>();
        return true;
    }
    bool BoostSocketWrapperModule::ShutdownImpl()
    {
        g_network_accessor.Shutdown();
        return true;
    }
    bool BoostSocketWrapperModule::DynamicLoadable()
    {
        return true;
    }
} // namespace Engine

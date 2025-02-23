#include "ProjectionFrustumModule.h"

#include "EngineEntryPoint.h"
#include "ProjectionFrustum.h"
#include "ProjectionFrustumModule.generated.h"

bool Engine::ProjectionFrustumModule::InitializeImpl()
{
    CoreLoop::AddManager(CoreLoop::LOOP_TYPE_RENDER, &Managers::ProjectionFrustum::GetInstance);
    return true;   
}

bool Engine::ProjectionFrustumModule::ShutdownImpl()
{
    CoreLoop::RemoveManager(CoreLoop::LOOP_TYPE_RENDER, &Managers::ProjectionFrustum::GetInstance);
    return true;   
}

bool Engine::ProjectionFrustumModule::DynamicLoadable()
{
    return true;
}

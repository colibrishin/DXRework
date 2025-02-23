#include "TaskSchedulerModule.h"
#include "TaskSchedulerModule.generated.h"

#include "EngineEntryPoint.h"
#include "TaskScheduler.h"

MODULE_IMPL(Engine::TaskSchedulerModule, TaskScheduler)

bool Engine::TaskSchedulerModule::InitializeImpl()
{
    CoreLoop::AddManager(CoreLoop::LOOP_TYPE_LOGIC, &Managers::TaskScheduler::GetInstance);
    return true;
}

bool Engine::TaskSchedulerModule::ShutdownImpl()
{
    return true;
}

bool Engine::TaskSchedulerModule::DynamicLoadable()
{
    return true;
}

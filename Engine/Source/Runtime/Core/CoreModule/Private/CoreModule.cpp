#include "CoreModule/Public/CoreModule.h"
#include "CoreModule.generated.h"
#include "EngineEntryPoint.h"

MODULE_IMPL(Engine::CoreModule, Core)

bool Engine::CoreModule::InitializeImpl()
{
	CoreLoop::AddManager
			(
			 CoreLoop::LOOP_TYPE_LOGIC,
			 &Managers::ResourceManager::GetInstance,
			 &Managers::SceneManager::GetInstance
			);

#if WITH_DEBUG
	CoreLoop::AddManager
			(
			 CoreLoop::LOOP_TYPE_RENDER,
			 &Managers::Debugger::GetInstance
			);
#endif

	return true;
}

bool Engine::CoreModule::ShutdownImpl()
{
	CoreLoop::RemoveManager
			(
			 CoreLoop::LOOP_TYPE_LOGIC,
			 &Managers::ResourceManager::GetInstance,
			 &Managers::SceneManager::GetInstance
			);

#if WITH_DEBUG
	CoreLoop::RemoveManager
			(
			 CoreLoop::LOOP_TYPE_RENDER,
			 &Managers::Debugger::GetInstance
			);
#endif
	
#if WITH_EDITOR
	UIInterfaceAccessor::Shutdown();
#endif
	return true;
}

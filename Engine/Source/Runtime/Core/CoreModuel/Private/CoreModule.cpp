#include "CoreModuel/Public/CoreModule.h"
#include "CoreModule.generated.h"
#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::CoreModule, Core);

UPDATE_CALL_TEMPLATE_OneParam(OnUIUpdate, Engine::UIContext* const, parent)
UPDATE_CALL_TEMPLATE(PreUpdate)
UPDATE_CALL_TEMPLATE(Update)
UPDATE_CALL_TEMPLATE(PostUpdate)
UPDATE_CALL_TEMPLATE(FixedUpdate)
UPDATE_CALL_TEMPLATE(PreRender)
UPDATE_CALL_TEMPLATE(Render)
UPDATE_CALL_TEMPLATE(PostRender)

Engine::CoreLoop Engine::CoreModule::s_core_module = {};

void Engine::CoreLoop::OnUIUpdate(UIContext* const parent, const float dt) const
{
	for (const auto& singletons : m_singleton_accessor_)
	{
		DoOnUIUpdate(parent, dt, singletons);
	}
}

void Engine::CoreLoop::PreUpdate(const float dt) const
{
	for (const auto& singletons : m_singleton_accessor_)
	{
		DoPreUpdate(dt, singletons);	
	}
}

void Engine::CoreLoop::Update(const float dt) const
{
	for (const auto& singletons : m_singleton_accessor_)
	{
		DoUpdate(dt, singletons);	
	}
}

void Engine::CoreLoop::PostUpdate(const float dt) const
{
	for (const auto& singletons : m_singleton_accessor_)
	{
		DoPostUpdate(dt, singletons);	
	}
}

void Engine::CoreLoop::FixedUpdate(const float dt) const
{
	for (const auto& singletons : m_singleton_accessor_)
	{
		DoFixedUpdate(dt, singletons);	
	}
}

void Engine::CoreLoop::PreRender(const float dt) const
{
	for (const auto& singletons : m_singleton_accessor_)
	{
		DoPreRender(dt, singletons);	
	}
}

void Engine::CoreLoop::Render(const float dt) const
{
	for (const auto& singletons : m_singleton_accessor_)
	{
		DoRender(dt, singletons);	
	}
}

void Engine::CoreLoop::PostRender(const float dt) const
{
	for (const auto& singletons : m_singleton_accessor_)
	{
		DoPostRender(dt, singletons);	
	}
}

bool Engine::CoreModule::InitializeImpl()
{
	s_core_module.AddManager
			(
			 CoreLoop::LOOP_TYPE_LOGIC,
			 &Managers::ResourceManager::GetInstance,
			 &Managers::SceneManager::GetInstance,
			 &Managers::TaskScheduler::GetInstance,
			 &Managers::CameraManager::GetInstance
			);

#if WITH_DEBUG
	s_core_module.AddManager
			(
			 CoreLoop::LOOP_TYPE_RENDER,
			 &Managers::Debugger::GetInstance
			);
#endif

	return true;
}

bool Engine::CoreModule::ShutdownImpl()
{
	s_core_module.RemoveManager
			(
			 CoreLoop::LOOP_TYPE_LOGIC,
			 &Managers::ResourceManager::GetInstance,
			 &Managers::SceneManager::GetInstance,
			 &Managers::TaskScheduler::GetInstance,
			 &Managers::CameraManager::GetInstance
			);

#if WITH_DEBUG
	s_core_module.RemoveManager
			(
			 CoreLoop::LOOP_TYPE_RENDER,
			 &Managers::Debugger::GetInstance
			);
#endif

	GraphicInterfaceAccessor::Shutdown();
	UIInterfaceAccessor::Shutdown();

	return true;
}

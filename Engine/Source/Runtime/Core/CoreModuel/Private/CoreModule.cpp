#include "CoreModuel/Public/CoreModule.h"
#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::CoreModule, Core);

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

void Engine::CoreModule::Initialize()
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
}

void Engine::CoreModule::Shutdown()
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
}

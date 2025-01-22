#include "CoreModuel/Public/CoreModule.h"
#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::CoreModule, Core);

Engine::CoreLoop Engine::CoreModule::s_core_module = {};

void Engine::CoreLoop::PreUpdate(const float dt) const
{
	DoPreUpdate(dt, m_singleton_accessor_);
}

void Engine::CoreLoop::Update(const float dt) const
{
	DoUpdate(dt, m_singleton_accessor_);
}

void Engine::CoreLoop::PostUpdate(const float dt) const
{
	DoPostUpdate(dt, m_singleton_accessor_);
}

void Engine::CoreLoop::FixedUpdate(const float dt) const
{
	DoFixedUpdate(dt, m_singleton_accessor_);
}

void Engine::CoreLoop::PreRender(const float dt) const
{
	DoPreRender(dt, m_singleton_accessor_);
}

void Engine::CoreLoop::Render(const float dt) const
{
	DoRender(dt, m_singleton_accessor_);
}

void Engine::CoreLoop::PostRender(const float dt) const
{
	DoPostRender(dt, m_singleton_accessor_);
}

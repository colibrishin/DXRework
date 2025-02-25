#include "EngineEntryPoint.h"

#include "ModuleManager.h"

#if WITH_EDITOR
#include "UIInterface.h"
#endif

SingletonCollection Engine::CoreLoop::m_singleton_accessor_[LOOP_TYPE_MAX] = {};
bool Engine::Managers::EngineEntryPoint::s_instantiated_ = false;
std::atomic<bool> Engine::Managers::EngineEntryPoint::s_paused = false;
std::atomic<float> Engine::Managers::EngineEntryPoint::s_fixed_update_interval = 1 / 30.f;


#if WITH_EDITOR
UPDATE_CALL_TEMPLATE_OneParam(OnUIUpdate, Engine::UIContext* const, parent)
#endif
UPDATE_CALL_TEMPLATE(PreUpdate)
UPDATE_CALL_TEMPLATE(Update)
UPDATE_CALL_TEMPLATE(PostUpdate)
UPDATE_CALL_TEMPLATE(FixedUpdate)
UPDATE_CALL_TEMPLATE(PreRender)
UPDATE_CALL_TEMPLATE(Render)
UPDATE_CALL_TEMPLATE(PostRender)

#if WITH_EDITOR
void Engine::CoreLoop::OnUIUpdate(UIContext* const parent, const float dt)
{
    for (const auto& singletons : m_singleton_accessor_)
    {
        DoOnUIUpdate(parent, dt, singletons);
    }
}
#endif

void Engine::CoreLoop::PreUpdate(const float dt)
{
    for (const auto& singletons : m_singleton_accessor_)
    {
        DoPreUpdate(dt, singletons);	
    }
}

void Engine::CoreLoop::Update(const float dt)
{
    for (const auto& singletons : m_singleton_accessor_)
    {
        DoUpdate(dt, singletons);	
    }
}

void Engine::CoreLoop::PostUpdate(const float dt)
{
    for (const auto& singletons : m_singleton_accessor_)
    {
        DoPostUpdate(dt, singletons);	
    }
}

void Engine::CoreLoop::FixedUpdate(const float dt)
{
    for (const auto& singletons : m_singleton_accessor_)
    {
        DoFixedUpdate(dt, singletons);	
    }
}

void Engine::CoreLoop::PreRender(const float dt)
{
    for (const auto& singletons : m_singleton_accessor_)
    {
        DoPreRender(dt, singletons);	
    }
}

void Engine::CoreLoop::Render(const float dt)
{
    for (const auto& singletons : m_singleton_accessor_)
    {
        DoRender(dt, singletons);	
    }
}

void Engine::CoreLoop::PostRender(const float dt)
{
    for (const auto& singletons : m_singleton_accessor_)
    {
        DoPostRender(dt, singletons);	
    }
}

namespace Engine::Managers
{
	EngineEntryPoint::EngineEntryPoint(SINGLETON_LOCK_TOKEN)
		: Singleton()
	{
		if (s_instantiated_)
		{
			throw std::runtime_error("EngineEntryPoint is already instantiated");
		}

		s_instantiated_ = true;
		std::set_terminate(SIGTERM);
	}

	float EngineEntryPoint::GetDeltaTime() const
	{
		return static_cast<float>(m_timer->GetElapsedSeconds());
	}

	uint32_t EngineEntryPoint::GetFPS() const
	{
		return m_timer->GetFramesPerSecond();
	}

	EngineEntryPoint::~EngineEntryPoint()
	{
		SIGTERM();
	}

	void EngineEntryPoint::Initialize()
	{
		m_timer = std::make_unique<DX::StepTimer>();
		ModuleManager::GetInstance().Initialize();
		ModuleManager::GetInstance().LoadModuleAll();
	}

	void EngineEntryPoint::Tick()
	{
		static auto internal_tick = std::bind_front(&EngineEntryPoint::tickInternal, this);
		m_timer->Tick(internal_tick);
	}

#if WITH_EDITOR
	void EngineEntryPoint::OnUIUpdate(UIContext* const parent, const float dt)
	{
		CoreLoop::OnUIUpdate(parent, dt);
	}
#endif

	void EngineEntryPoint::PreUpdate(const float dt)
	{
		CoreLoop::PreUpdate(dt);
	}

	void EngineEntryPoint::FixedUpdate(const float dt)
	{
		CoreLoop::FixedUpdate(dt);
	}

	void EngineEntryPoint::Update(const float dt)
	{
		CoreLoop::Update(dt);
	}

	void EngineEntryPoint::PreRender(const float dt)
	{
		CoreLoop::PreRender(dt);
	}

	void EngineEntryPoint::Render(const float dt)
	{
		CoreLoop::Render(dt);
	}

	void EngineEntryPoint::PostRender(const float dt)
	{
		CoreLoop::PostRender(dt);
	}

	void EngineEntryPoint::PostUpdate(const float dt)
	{
		CoreLoop::PostUpdate(dt);
	}

	void EngineEntryPoint::tickInternal()
	{
		static float elapsed = 0.f;

		float dt = GetDeltaTime();

		if (s_paused)
		{
			elapsed = 0.f;
			dt      = 0.f;
		}

#if WITH_EDITOR
		if (UIInterfaceAccessor::IsValid())
		{
			UIInterfaceAccessor::NewFrame();
		}
#endif
		
		while (elapsed >= s_fixed_update_interval)
		{
			FixedUpdate(s_fixed_update_interval);
			elapsed -= s_fixed_update_interval;
		}

#if WITH_EDITOR
		if (UIInterfaceAccessor::IsValid())
		{
			OnUIUpdate(nullptr, dt);
		}
#endif
		
		PreUpdate(dt);
		Update(dt);
		PostUpdate(dt);

		PreRender(dt);
		Render(dt);
		PostRender(dt);

		elapsed += dt;
	}

	void EngineEntryPoint::SIGTERM()
	{
		ModuleManager::GetInstance().Destroy();
	}
} // namespace Engine::Manager

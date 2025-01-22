#include "../Public/EngineEntryPoint.h"

#include "UIInterface.h"

#include "CoreModuel/Public/CoreModule.h"
#include "Source/Runtime/Core/ModuleManager/Public/ModuleManager.h"

bool Engine::Managers::EngineEntryPoint::s_instantiated_ = false;
std::atomic<bool> Engine::Managers::EngineEntryPoint::s_paused = false;
std::atomic<float> Engine::Managers::EngineEntryPoint::s_fixed_update_interval = 1 / 30.f;

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
		ModuleManager::GetInstance().LoadModule(L"Core");
		ModuleManager::GetInstance().LoadModule(L"D3D12GraphicInterface");
		ModuleManager::GetInstance().LoadModule(L"RenderPipeline");
		ModuleManager::GetInstance().LoadModule(L"ImGuiManager");
		ModuleManager::GetInstance().LoadModule(L"PhysicsManager");
		ModuleManager::GetInstance().LoadModule(L"Material");
		ModuleManager::GetInstance().LoadModule(L"Animator");
		ModuleManager::GetInstance().LoadModule(L"ModelRenderer");
		ModuleManager::GetInstance().LoadModule(L"ParticleRenderer");
		ModuleManager::GetInstance().LoadModule(L"Texture2D");
		ModuleManager::GetInstance().LoadModule(L"Shader");
		ModuleManager::GetInstance().LoadModule(L"Shape");
		ModuleManager::GetInstance().LoadModule(L"AtlasAnimationTexture");
	}

	void EngineEntryPoint::Tick()
	{
		static auto internal_tick = std::bind_front(&EngineEntryPoint::tickInternal, this);
		m_timer->Tick(internal_tick);
	}

	void EngineEntryPoint::OnUIUpdate(UIContext* const parent, const float dt)
	{
		CoreModule::GetContext().OnUIUpdate(parent, dt);
	}

	void EngineEntryPoint::PreUpdate(const float dt)
	{
		CoreModule::GetContext().PreUpdate(dt);
	}

	void EngineEntryPoint::FixedUpdate(const float dt)
	{
		CoreModule::GetContext().FixedUpdate(dt);
	}

	void EngineEntryPoint::Update(const float dt)
	{
		CoreModule::GetContext().Update(dt);
	}

	void EngineEntryPoint::PreRender(const float dt)
	{
		CoreModule::GetContext().PreRender(dt);
	}

	void EngineEntryPoint::Render(const float dt)
	{
		CoreModule::GetContext().Render(dt);
	}

	void EngineEntryPoint::PostRender(const float dt)
	{
		CoreModule::GetContext().PostRender(dt);
	}

	void EngineEntryPoint::PostUpdate(const float dt)
	{
		CoreModule::GetContext().PostUpdate(dt);
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

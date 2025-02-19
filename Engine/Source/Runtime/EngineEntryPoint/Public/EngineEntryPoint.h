#pragma once
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"
#include "Source/Runtime/Core/StepTimer/Public/StepTimer.hpp"

#include "EngineEntryPoint.generated.h"

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_ENGINEENTRYPOINT_API EngineEntryPoint final : public Abstracts::Singleton<EngineEntryPoint>
	{
		GENERATE_BODY
	public:
		EngineEntryPoint(SINGLETON_LOCK_TOKEN);

		void        Initialize() override;
		void        Tick();

		float           GetDeltaTime() const;
		uint32_t        GetFPS() const;

	private:
		friend struct SingletonDeleter;
		~EngineEntryPoint() override;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

		void PreUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;

		void tickInternal();

		static void SIGTERM();

		HWND m_hWnd = nullptr;

		// Time
		std::unique_ptr<DX::StepTimer> m_timer;

		// Check for Sigterm registration
		static bool s_instantiated_;
		static std::atomic<bool> s_paused;
		static std::atomic<float> s_fixed_update_interval;
	};
} // namespace Engine::Manager

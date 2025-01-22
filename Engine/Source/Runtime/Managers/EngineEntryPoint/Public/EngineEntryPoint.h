#pragma once
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"
#include "Source/Runtime/Managers/StepTimer/Public/StepTimer.hpp"

namespace Engine::Managers
{
	class EngineEntryPoint final : public Abstracts::Singleton<EngineEntryPoint>
	{
	public:
		EngineEntryPoint(SINGLETON_LOCK_TOKEN);

		void        Initialize() override;
		static void UpdateWindowSize();
		void        Tick();

		float           GetDeltaTime() const;
		uint32_t        GetFPS() const;
		uint32_t        GetWidth() const;
		uint32_t        GetHeight() const;
		bool            IsFullScreen() const;

	private:
		friend struct SingletonDeleter;
		~EngineEntryPoint() override;

		void PreUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void tickInternal();

		static void SIGTERM();

		HWND m_hWnd = nullptr;

		// Time
		std::unique_ptr<DX::StepTimer> m_timer;

		// Check for Sigterm registration
		static bool s_instantiated_;
		static std::atomic<bool> s_paused;
		static std::atomic<bool> s_fullscreen;
		static std::atomic<float> s_fixed_update_interval;
	};
} // namespace Engine::Manager

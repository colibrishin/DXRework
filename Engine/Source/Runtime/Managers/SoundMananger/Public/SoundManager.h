#pragma once
#include "Source/ThirdParty/FMOD/Public/fmod.hpp"
#include "Source/Runtime/Abstracts/CoreSingleton/Public/Singleton.hpp"

namespace FMOD 
{
	// Helper class for COM exceptions
	class fmod_exception : public std::exception
	{
	public:
		fmod_exception(FMOD_RESULT hr)
			: result(hr) {}

		const char* what() const noexcept override;

	private:
		FMOD_RESULT result;
	};
}

namespace Engine::Managers
{
	class SoundManager final : public Abstracts::Singleton<SoundManager>
	{
	public:
		explicit SoundManager(SINGLETON_LOCK_TOKEN) {}

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void LoadSound(FMOD::Sound** sound, const std::string& path) const;
		void PlaySound(
			FMOD::Sound* sound, const FMOD_VECTOR& pos,
			const FMOD_VECTOR& vel, FMOD::Channel** channel
		) const;
		void StopSound(FMOD::Sound* sound, FMOD::Channel** channel) const;

		void Set3DListener(
			const FMOD_VECTOR& position, const FMOD_VECTOR& velocity,
			const FMOD_VECTOR& forward, const FMOD_VECTOR& up
		) const;

	private:
		friend struct SingletonDeleter;
		~SoundManager() override;

		FMOD::System* m_audio_engine_ = nullptr;
		FMOD::ChannelGroup* m_master_channel_group_ = nullptr;
		FMOD::ChannelControl* m_channel_control_ = nullptr;
	};
} // namespace Engine::Managers

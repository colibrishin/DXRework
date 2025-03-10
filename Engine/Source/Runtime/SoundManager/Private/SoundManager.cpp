#include "SoundManager.h"
#include "ISoundAPI.h"

namespace Engine::Managers
{
	SoundManager::~SoundManager()
	{
		g_sound_accessor.Shutdown();
	}

	void SoundManager::Initialize() {}

	void SoundManager::PreUpdate(const float dt) {}

	void SoundManager::Update(const float dt) {}

	void SoundManager::PreRender(const float dt) {}

	void SoundManager::Render(const float dt) {}

	void SoundManager::PostRender(const float dt) {}

	void SoundManager::FixedUpdate(const float dt) { }

	void SoundManager::PostUpdate(const float dt)
	{
		g_sound_accessor.GetInterface().Update();
	}
} // namespace Engine::Manager::Graphics

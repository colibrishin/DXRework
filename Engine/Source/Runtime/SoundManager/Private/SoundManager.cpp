#include "SoundManager.h"
#include "ISoundAPI.h"

namespace Engine::Managers
{
	SoundManager::~SoundManager()
	{
		s_sa.Shutdown();
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
		s_sa.GetInterface().Update();
	}
} // namespace Engine::Manager::Graphics

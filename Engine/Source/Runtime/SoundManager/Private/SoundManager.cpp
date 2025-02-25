#include "SoundManager.h"
#include "SoundInterface.h"

namespace Engine::Managers
{
	SoundManager::~SoundManager()
	{
		SoundInterfaceAccessor::GetInterface().Shutdown();
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
		SoundInterfaceAccessor::GetInterface().Update();
	}
} // namespace Engine::Manager::Graphics

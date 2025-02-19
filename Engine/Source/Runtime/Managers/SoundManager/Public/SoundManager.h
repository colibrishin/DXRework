#pragma once
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"

#include "SoundManager.generated.h"

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_SOUNDMANAGER_API SoundManager final : public Abstracts::Singleton<SoundManager>
	{
		GENERATE_BODY
	public:
		explicit SoundManager(SINGLETON_LOCK_TOKEN) {}

		void Initialize() override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

	private:
		friend struct SingletonDeleter;
		~SoundManager() override;
	};
} // namespace Engine::Managers

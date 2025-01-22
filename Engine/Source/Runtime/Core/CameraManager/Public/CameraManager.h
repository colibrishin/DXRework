#pragma once
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

#include "CameraManager.generated.h"

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_CORE_API CameraManager : public Abstracts::Singleton<CameraManager>
	{
		GENERATE_BODY
	public:
		CameraManager(SINGLETON_LOCK_TOKEN)
			: Singleton() {}

		void Initialize() override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;

	private:
		friend struct SingletonDeleter;
		~CameraManager() override = default;
	};
} // namespace Engine::Managers

#pragma once
#include "Source/Runtime/CoreSingleton/Public/Singleton.hpp"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

POLYMORPHIC_MANAGER_TYPE_MAP(Engine::Managers::CameraManager)

namespace Engine::Managers
{
	class ENGINE_CORE_API CameraManager : public Abstracts::Singleton<CameraManager>
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(CameraManager)

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

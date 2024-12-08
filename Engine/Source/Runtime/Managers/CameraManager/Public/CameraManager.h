#pragma once
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine::Managers
{
	class CameraManager : public Abstracts::Singleton<CameraManager>
	{
	public:
		CameraManager(SINGLETON_LOCK_TOKEN)
			: Singleton() {}

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		Vector2 GetWorldMousePosition() const;

	private:
		friend struct SingletonDeleter;
		~CameraManager() override = default;
	};
} // namespace Engine::Managers

#pragma once
#include "egManager.hpp"

namespace Engine::Component
{
	class Rigidbody;
}

namespace Engine::Manager::Physics
{
	class PhysicsManager : public Abstract::Singleton<PhysicsManager>
	{
	public:
		explicit PhysicsManager(SINGLETON_LOCK_TOKEN) : Singleton()
		{
		}

		~PhysicsManager() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;

	private:
		static void UpdateGravity(Engine::Component::Rigidbody* rb);
		static void UpdateObject(Component::Rigidbody* rb, const float& dt);

	};
}

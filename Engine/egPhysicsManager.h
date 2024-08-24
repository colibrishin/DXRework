#pragma once
#include "egManager.hpp"

#ifdef PHYSX_ENABLED
namespace physx
{
	class PxFoundation;
}
#endif

namespace Engine::Manager::Physics
{
	class PhysicsManager : public Abstract::Singleton<PhysicsManager>
	{
	public:
		explicit PhysicsManager(SINGLETON_LOCK_TOKEN)
			: Singleton() {}

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

	private:
		friend struct SingletonDeleter;
		~PhysicsManager() override = default;

		static void EpsilonGuard(Vector3& lvel);
		static void UpdateObject(Components::Rigidbody* rb, const float& dt);

#ifdef PHYSX_ENABLED
		physx::PxFoundation* m_physx_foundation_;
#endif

	};
} // namespace Engine::Manager::Physics


REGISTER_TYPE(Engine::Manager::Application, Engine::Manager::Physics::PhysicsManager)

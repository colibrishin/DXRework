#pragma once

#include "egManager.hpp"

#ifdef PHYSX_ENABLED
namespace physx
{
	class PxCpuDispatcher;
	class PxPvd;
	class PxPhysics;
	class PxFoundation;
	class PxCudaContextManager;
	class PxCooking;
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
		~PhysicsManager() override;

		static void EpsilonGuard(Vector3& lvel);
		static void UpdateObject(Components::Rigidbody* rb, const float& dt);

#ifdef PHYSX_ENABLED
	private:
		physx::PxFoundation* m_px_foundation_;
		physx::PxPvd* m_px_pvd_;
		physx::PxPhysics* m_px_;
		physx::PxCudaContextManager* m_context_manager_;
		physx::PxCpuDispatcher* m_px_cpu_dispatcher_;

	public:
		[[nodiscard]] physx::PxPhysics* GetPhysX() const
		{
			return m_px_;
		}

		[[nodiscard]] physx::PxCudaContextManager* GetCudaContext() const
		{
			return m_context_manager_;
		}

		[[nodiscard]] physx::PxCpuDispatcher* GetCPUDispatcher() const
		{
			return m_px_cpu_dispatcher_;
		}
#endif

	};
} // namespace Engine::Manager::Physics


REGISTER_TYPE(Engine::Manager::Application, Engine::Manager::Physics::PhysicsManager)

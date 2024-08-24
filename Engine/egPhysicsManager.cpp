#include "pch.h"
#include "egPhysicsManager.h"

#include "egBaseCollider.hpp"
#include "egCollision.h"
#include "egElastic.h"
#include "egFriction.h"
#include "egManagerHelper.hpp"
#include "egObject.hpp"
#include "egPhysics.hpp"
#include "egRigidbody.h"
#include "egSceneManager.hpp"
#include "egTransform.h"

#ifdef PHYSX_ENABLED
#include <PxPhysicsAPI.h>
#pragma comment(lib, "LowLevel_static_64.lib")
#pragma comment(lib, "LowLevelAABB_static_64.lib")
#pragma comment(lib, "LowLevelDynamics_static_64.lib")
#pragma comment(lib, "PhysX_64.lib")
#pragma comment(lib, "PhysXCharacterKinematic_static_64.lib")
#pragma comment(lib, "PhysXCommon_64.lib")
#pragma comment(lib, "PhysXCooking_64.lib")
#pragma comment(lib, "PhysXExtensions_static_64.lib")
#pragma comment(lib, "PhysXFoundation_64.lib")
#pragma comment(lib, "PhysXPvdSDK_static_64.lib")
#pragma comment(lib, "PhysXTask_static_64.lib")
#pragma comment(lib, "PhysXVehicle_static_64.lib")
#pragma comment(lib, "PhysXVehicle2_static_64.lib")
#pragma comment(lib, "SceneQuery_static_64.lib")
#pragma comment(lib, "SimulationController_static_64.lib")
#endif

namespace Engine::Manager::Physics
{
	void PhysicsManager::Initialize()
	{
#ifdef PHYSX_ENABLED
		if (!m_physx_foundation_)
		{
			static physx::PxDefaultErrorCallback s_error_callback;
			static physx::PxDefaultAllocator s_allocator;

			m_physx_foundation_ = PxCreateFoundation(PX_PHYSICS_VERSION, s_allocator, s_error_callback);

			if (!m_physx_foundation_)
			{
				throw std::exception("Unable to initialize physx!");
			}
		}
#endif
	}

	void PhysicsManager::PreUpdate(const float& dt) {}

	void PhysicsManager::Update(const float& dt) {}

	void PhysicsManager::PreRender(const float& dt) {}

	void PhysicsManager::Render(const float& dt) {}

	void PhysicsManager::PostRender(const float& dt) {}

	void PhysicsManager::FixedUpdate(const float& dt)
	{
		if (const auto scene = GetSceneManager().GetActiveScene().lock())
		{
			const auto& rbs = scene->GetCachedComponents<Components::Rigidbody>();

			for (const auto rb : rbs)
			{
				if (const auto locked = rb.lock())
				{
					UpdateObject(locked->GetSharedPtr<Components::Rigidbody>().get(), dt);
				}
			}
		}
	}

	void PhysicsManager::PostUpdate(const float& dt) {}

	void PhysicsManager::EpsilonGuard(Vector3& lvel)
	{
		if (lvel.x < g_epsilon && lvel.x > -g_epsilon)
		{
			lvel.x = 0.0f;
		}
		if (lvel.y < g_epsilon && lvel.y > -g_epsilon)
		{
			lvel.y = 0.0f;
		}
		if (lvel.z < g_epsilon && lvel.z > -g_epsilon)
		{
			lvel.z = 0.0f;
		}
	}

	void PhysicsManager::UpdateObject(Components::Rigidbody* rb, const float& dt)
	{
		if (rb->IsFixed())
		{
			return;
		}

		const auto& cl =
				rb->GetOwner().lock()->GetComponent<Components::Collider>().lock();
		const auto t1 = rb->GetT1();

		Vector3 lvel = rb->GetT0LinearVelocity();

		const Vector3 lfrc = Engine::Physics::EvalFriction
				(
				 lvel, rb->GetFrictionCoefficient(),
				 dt
				);

		const Vector3 rvel = rb->GetT0AngularVelocity();

		lvel += lfrc;
		Engine::Physics::FrictionVelocityGuard(lvel, lfrc);

		EpsilonGuard(lvel);

		t1->SetLocalPosition
				(
				 t1->GetLocalPosition() + Engine::Physics::EvalT1PositionDelta(lvel, rb->GetT0Force(), dt)
				);

		if (!rb->GetNoAngular())
		{
			Quaternion orientation = t1->GetLocalRotation();
			orientation += Quaternion{
				Engine::Physics::EvalT1PositionDelta
				(
				 rvel, rb->GetT0Torque(), dt
				),
				0.0f
			} * orientation;

			orientation.Normalize();
			t1->SetLocalRotation(orientation);
			rb->SetT0AngularVelocity
					(
					 Engine::Physics::EvalT1Velocity(rvel, rb->GetT0Torque(), rb->GetT1Torque(), dt)
					);
		}

		rb->SetT0LinearVelocity
				(
				 Engine::Physics::EvalT1Velocity(lvel, rb->GetT0Force(), rb->GetT1Force(), dt)
				);

		rb->Reset();

		rb->SetLinearFriction(lfrc);
	}
} // namespace Engine::Manager::Physics

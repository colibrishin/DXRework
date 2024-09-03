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
#include <cooking/PxCooking.h>
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
		if (!m_px_foundation_)
		{
			static physx::PxDefaultErrorCallback s_error_callback;
			static physx::PxDefaultAllocator s_allocator;

			m_px_foundation_ = PxCreateFoundation(PX_PHYSICS_VERSION, s_allocator, s_error_callback);

			if (!m_px_foundation_)
			{
				throw std::exception("Unable to initialize physx foundation!");
			}

			if constexpr (g_debug)
			{
				m_px_pvd_ = physx::PxCreatePvd(*m_px_foundation_);
				physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
				m_px_pvd_->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
			}

			m_px_ = PxCreatePhysics(PX_PHYSICS_VERSION, *m_px_foundation_, physx::PxTolerancesScale(), g_debug, m_px_pvd_);

			if (!m_px_)
			{
				throw std::exception("Unable to initialize physx physics!");
			}

			const physx::PxCudaContextManagerDesc context_desc{};
			m_context_manager_ = PxCreateCudaContextManager(
				*m_px_foundation_, 
				context_desc,
				PxGetProfilerCallback());

			if (!m_context_manager_)
			{
				throw std::exception("Unable to initialize cuda context!");
			}

			m_px_cpu_dispatcher_ = physx::PxDefaultCpuDispatcherCreate(std::thread::hardware_concurrency());

			if (!m_px_cpu_dispatcher_)
			{
				throw std::exception("Unable to initialize cpu dispatcher for physx!");
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
#ifdef PHYSX_ENABLED
			scene->GetPhysXScene()->advance();
			scene->GetPhysXScene()->fetchResults(true);
			UpdateFromPhysX();
#else
			const auto& rbs = scene->GetCachedComponents<Components::Rigidbody>();

			for (const auto rb : rbs)
			{
				if (const auto locked = rb.lock())
				{
					UpdateObject(locked->GetSharedPtr<Components::Rigidbody>().get(), dt);
				}
			}
#endif
		}
	}

	void PhysicsManager::PostUpdate(const float& dt) {}

	PhysicsManager::~PhysicsManager()
	{
#ifdef PHYSX_ENABLED
		if (m_px_)
		{
			m_px_->release();
			m_px_ = nullptr;
		}

		if (g_debug)
		{
			if (m_px_pvd_)
			{
				m_px_pvd_->release();
				m_px_pvd_ = nullptr;
			}
		}

		if (m_px_foundation_)
		{
			m_px_foundation_->release();
			m_px_foundation_ = nullptr;
		}
#endif
	}

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
				1.0f
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

#ifdef PHYSX_ENABLED
	void PhysicsManager::UpdateFromPhysX()
	{
		if (const auto& scene = GetSceneManager().GetActiveScene().lock())
		{
			const physx::PxScene* px_scene = scene->GetPhysXScene();
			_ASSERT(px_scene);

			std::vector<physx::PxActor*> px_actors;

			const physx::PxU32 px_dynamic_actor_count = px_scene->getNbActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC);
			px_actors.resize(px_dynamic_actor_count);

			px_scene->getActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC, px_actors.data(), px_dynamic_actor_count);

			for (const physx::PxActor* const& actor : px_actors)
			{
				const auto px_dynamic = actor->is<physx::PxRigidDynamic>();
				_ASSERT(px_dynamic);

				const auto& internal_collider = static_cast<Components::Collider*>(px_dynamic->userData);

				if (const StrongObjectBase& owner = internal_collider->GetOwner().lock())
				{
					const StrongTransform& internal_transform = owner->GetComponent<Components::Transform>().lock();
					_ASSERT(internal_transform);

					const physx::PxTransform& position = px_dynamic->getGlobalPose();
					internal_transform->SetWorldPosition(reinterpret_cast<const Vector3&>(position.p));
					internal_transform->SetWorldRotation(reinterpret_cast<const Quaternion&>(position.q));

					if (const StrongRigidbody& internal_rigidbody = owner->GetComponent<Components::Rigidbody>().lock())
					{
						const physx::PxVec3& linear_velocity = px_dynamic->getLinearVelocity();
						const physx::PxVec3& angular_velocity = px_dynamic->getAngularVelocity();

						internal_rigidbody->m_linear_velocity = reinterpret_cast<const Vector3&>(linear_velocity);
						internal_rigidbody->m_angular_velocity = reinterpret_cast<const Vector3&>(angular_velocity);

						internal_rigidbody->Reset();
						internal_rigidbody->Synchronize();
					}
				}
			}
		}
	}
#endif
} // namespace Engine::Manager::Physics

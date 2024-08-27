#include "pch.h"
#ifdef PHYSX_ENABLED
#include "PhysXSimulationCallback.h"

#include <PxRigidActor.h>
#include <PxShape.h>

#include "egBaseCollider.hpp"
#include "egCollisionDetector.h"

namespace Engine::Physics
{
	void PhysXSimulationCallback::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
	{
	}

	void PhysXSimulationCallback::onWake(physx::PxActor** actors, physx::PxU32 count)
	{
	}

	void PhysXSimulationCallback::onSleep(physx::PxActor** actors, physx::PxU32 count)
	{
	}

	void PhysXSimulationCallback::onContact(const physx::PxContactPairHeader& pairHeader,
		const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
	{
		auto& frame_collision = GetCollisionDetector().m_frame_collision_map_;
		auto& collision_map = GetCollisionDetector().m_collision_map_;

		for (int i = 0; i < nbPairs; ++i)
		{
			physx::PxShape* const* shapes = pairs[i].shapes;

			const physx::PxRigidActor* px_lhs = shapes[0]->getActor();
			const physx::PxRigidActor* px_rhs = shapes[1]->getActor();

			const auto& lhs = static_cast<Components::Collider*>(px_lhs->userData);
			const auto& rhs = static_cast<Components::Collider*>(px_rhs->userData);

			if (!collision_map.contains(lhs->GetID()) ||
				!collision_map.at(lhs->GetID()).contains(rhs->GetID()))
			{
				frame_collision[lhs->GetID()].insert(rhs->GetID());
				frame_collision[rhs->GetID()].insert(lhs->GetID());

				lhs->onCollisionEnter.Broadcast(rhs->GetSharedPtr<Components::Collider>());
				rhs->onCollisionEnter.Broadcast(lhs->GetSharedPtr<Components::Collider>());
			}
		}

		for (const auto& [lhs, rhs_set] : collision_map)
		{
			if (!frame_collision.contains(lhs))
			{
				// no collision
				// remove from map
				continue;
			}

			for (const auto& rhs : rhs_set)
			{
				if (!frame_collision.at(lhs).contains(rhs))
				{
					// no collision
					// remove from set
				}
			}
		}

		for (const auto& [lhs, rhs_set] : frame_collision)
		{
			collision_map[lhs].insert(rhs_set.begin(), rhs_set.end());

			for (const auto& rhs : rhs_set)
			{
				collision_map[rhs].insert(lhs);
			}
		}
	}

	void PhysXSimulationCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
	{
	}

	void PhysXSimulationCallback::onAdvance(const physx::PxRigidBody* const* bodyBuffer,
		const physx::PxTransform* poseBuffer, const physx::PxU32 count)
	{
	}
}
#endif

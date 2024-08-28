#include "pch.h"

#include <extensions/PxDefaultSimulationFilterShader.h>
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
		HELPME
	}

	void PhysXSimulationCallback::onWake(physx::PxActor** actors, physx::PxU32 count)
	{
		HELPME
	}

	void PhysXSimulationCallback::onSleep(physx::PxActor** actors, physx::PxU32 count)
	{
		HELPME
	}

	void PhysXSimulationCallback::onContact(const physx::PxContactPairHeader& pairHeader,
		const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
	{
		auto& frame_collision = GetCollisionDetector().m_frame_collision_map_;
		auto& collision_map = GetCollisionDetector().m_collision_map_;

		for (int i = 0; i < nbPairs; ++i)
		{
			if (pairs[i].events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
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
		HELPME
	}

	void PhysXSimulationCallback::onAdvance(const physx::PxRigidBody* const* bodyBuffer,
		const physx::PxTransform* poseBuffer, const physx::PxU32 count)
	{
		HELPME
	}

	physx::PxFilterFlags SimulationFilterShader(
		physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, physx::PxPairFlags& pairFlags,
		const void*                     constantBlock, physx::PxU32      constantBlockSize
	)
	{
		PX_UNUSED(constantBlock);
		PX_UNUSED(constantBlockSize);
		// let triggers through
        if(physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
        {
                pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
                return physx::PxFilterFlag::eDEFAULT;
        }
        // generate contacts for all that were not filtered above
        pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

        // trigger the contact callback for pairs (A,B) where
        // the filtermask of A contains the ID of B and vice versa.
        if((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
                pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;

        return physx::PxFilterFlag::eDEFAULT;
	}
}
#endif

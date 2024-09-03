#include "pch.h"
#ifdef PHYSX_ENABLED

#include <PxScene.h>

#include <extensions/PxDefaultSimulationFilterShader.h>

#include "egPhysicsManager.h"
#include "egSceneManager.hpp"

#include "PhysXSimulationCallback.h"

#include <PxRigidActor.h>
#include <PxShape.h>

#include "egBaseCollider.hpp"
#include "egCollisionDetector.h"

namespace Engine::Physics
{
	PhysXSimulationFilterCallback g_filter_callback;
	PhysXSimulationCallback g_simulation_callback;

	void PhysXSimulationCallback::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
	{
	}

	void PhysXSimulationCallback::onWake(physx::PxActor** actors, physx::PxU32 count)
	{
	}

	void PhysXSimulationCallback::onSleep(physx::PxActor** actors, physx::PxU32 count)
	{
	}

	void PhysXSimulationCallback::onContact(
		const physx::PxContactPairHeader& pairHeader,
		const physx::PxContactPair* pairs, 
		physx::PxU32 nbPairs)
	{
	}

	void PhysXSimulationCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
	{
	}

	void PhysXSimulationCallback::onAdvance(const physx::PxRigidBody* const* bodyBuffer,
		const physx::PxTransform* poseBuffer, const physx::PxU32 count)
	{
	}

	physx::PxFilterFlags PhysXSimulationFilterCallback::pairFound(
		physx::PxU64 pairID, 
		physx::PxFilterObjectAttributes attributes0, 
		physx::PxFilterData filterData0,
		const physx::PxActor* a0, 
		const physx::PxShape* s0, 
		physx::PxFilterObjectAttributes attributes1,
		physx::PxFilterData filterData1, 
		const physx::PxActor* a1, 
		const physx::PxShape* s1,
		physx::PxPairFlags& pairFlags
	)
	{
		// this can be defined as static if collision detector is guaranteed not to be destroyed.
		auto& collision_map = GetCollisionDetector().m_collision_map_;

		if (pairFlags & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			const auto& lhs = static_cast<Components::Collider*>(a0->userData);
			const auto& rhs = static_cast<Components::Collider*>(a1->userData);

			if (!collision_map.contains(lhs->GetID()) ||
				!collision_map.at(lhs->GetID()).contains(rhs->GetID()))
			{
				collision_map[lhs->GetID()].insert(rhs->GetID());
				collision_map[rhs->GetID()].insert(lhs->GetID());

				lhs->onCollisionEnter.Broadcast(rhs->GetSharedPtr<Components::Collider>());
				rhs->onCollisionEnter.Broadcast(lhs->GetSharedPtr<Components::Collider>());

				lhs->AddCollidedObject(rhs->GetID());
				rhs->AddCollidedObject(lhs->GetID());
			}

			if (!m_pair_map_.contains(pairID))
			{
				m_pair_map_[pairID] = {a0, a1};
			}
		}

		// notify again for the pairLost
		return physx::PxFilterFlag::eNOTIFY;
	}

	void PhysXSimulationFilterCallback::pairLost(
		physx::PxU64 pairID, 
		physx::PxFilterObjectAttributes attributes0, 
		physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1, 
		physx::PxFilterData filterData1, 
		bool objectRemoved
	)
	{
		// this can be defined as static if collision detector is guaranteed not to be destroyed.
		auto& frame_collision = GetCollisionDetector().m_frame_collision_map_;
		auto& collision_map = GetCollisionDetector().m_collision_map_;

		_ASSERT(m_pair_map_.contains(pairID));

		const auto& collision_pair = m_pair_map_[pairID];

		const auto& lhs = static_cast<Components::Collider*>(collision_pair.first->userData);
		const auto& rhs = static_cast<Components::Collider*>(collision_pair.second->userData);

		if (collision_map.contains(lhs->GetID()) ||
			collision_map.at(lhs->GetID()).contains(rhs->GetID()))
		{
			collision_map[lhs->GetID()].erase(rhs->GetID());
			collision_map[rhs->GetID()].erase(lhs->GetID());

			lhs->onCollisionEnd.Broadcast(rhs->GetSharedPtr<Components::Collider>());
			rhs->onCollisionEnd.Broadcast(lhs->GetSharedPtr<Components::Collider>());

			lhs->RemoveCollidedObject(rhs->GetID());
			rhs->RemoveCollidedObject(lhs->GetID());
		}

		m_pair_map_.erase(pairID);

		return;
	}

	bool PhysXSimulationFilterCallback::statusChange(
		physx::PxU64& pairID, 
		physx::PxPairFlags& pairFlags, 
		physx::PxFilterFlags& filterFlags
	)
	{
		if (pairFlags.isSet(static_cast<physx::PxPairFlag::Enum>(0)) && filterFlags.isSet(static_cast<physx::PxFilterFlag::Enum>(0)))
		{
			return false;
		}

		return true;
	}

	physx::PxFilterFlags SimulationFilterShader(
		physx::PxFilterObjectAttributes attributes0,
		physx::PxFilterData             filterData0,
		physx::PxFilterObjectAttributes attributes1,
		physx::PxFilterData             filterData1,
		physx::PxPairFlags&             pairFlags,
		const void*                     constantBlock,
		physx::PxU32                    constantBlockSize
	)
	{
		PX_UNUSED(constantBlock);
		PX_UNUSED(constantBlockSize);

		// let triggers through
		if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
			return physx::PxFilterFlag::eDEFAULT;
		}

        // generate contacts for all that were not filtered above
        pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

        // trigger the contact callback for pairs (A,B) where
        // the filtermask of A contains the ID of B and vice versa.
        if((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
        {
	        pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
        }

        return physx::PxFilterFlag::eNOTIFY;
	}
}
#endif

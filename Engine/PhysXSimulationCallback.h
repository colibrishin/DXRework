#pragma once
#ifdef PHYSX_ENABLED
#include <PxSimulationEventCallback.h>
#include <foundation/PxSimpleTypes.h>

namespace Engine::Physics
{
	class PhysXSimulationCallback : public physx::PxSimulationEventCallback
	{
	public:
		void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;
		void onWake(physx::PxActor** actors, physx::PxU32 count) override;
		void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
		void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs,
			physx::PxU32 nbPairs) override;
		void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
		void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer,
			const physx::PxU32 count) override;
	};

	class PhysXSimulationFilterCallback : public physx::PxSimulationFilterCallback
	{
	public:
		physx::PxFilterFlags pairFound(
			physx::PxU64 pairID, physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
			const physx::PxActor* a0, const physx::PxShape* s0, physx::PxFilterObjectAttributes attributes1,
			physx::PxFilterData filterData1, const physx::PxActor* a1, const physx::PxShape* s1,
			physx::PxPairFlags& pairFlags
		) override;
		void pairLost(
			physx::PxU64 pairID, physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
			physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, bool objectRemoved
		) override;
		bool statusChange(
			physx::PxU64& pairID, physx::PxPairFlags& pairFlags, physx::PxFilterFlags& filterFlags
		) override;

	private:
		fast_pool_unordered_map<physx::PxU64, std::pair<const physx::PxActor*, const physx::PxActor*>> m_pair_map_{};
	};

	physx::PxFilterFlags SimulationFilterShader(
		physx::PxFilterObjectAttributes        attributes0,
		physx::PxFilterData             filterData0,
		physx::PxFilterObjectAttributes attributes1,
		physx::PxFilterData             filterData1,
		physx::PxPairFlags&             pairFlags,
		const void*                     constantBlock,
		physx::PxU32                    constantBlockSize);

	extern PhysXSimulationFilterCallback g_filter_callback;
	extern PhysXSimulationCallback g_simulation_callback;
}
#endif
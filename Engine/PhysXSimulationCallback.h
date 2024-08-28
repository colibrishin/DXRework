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

	physx::PxFilterFlags SimulationFilterShader(
		physx::PxFilterObjectAttributes        attributes0,
		physx::PxFilterData             filterData0,
		physx::PxFilterObjectAttributes attributes1,
		physx::PxFilterData             filterData1,
		physx::PxPairFlags&             pairFlags,
		const void*                     constantBlock,
		physx::PxU32                    constantBlockSize);

	inline PhysXSimulationCallback g_simulation_callback;
}
#endif
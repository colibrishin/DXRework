#pragma once
#include "Source/Runtime/TypeLibrary/Public/TypeLibrary.h"

namespace Engine::Physics
{
	inline static constexpr Vector3 g_gravity_vec = {0.f, -9.81f, 0.f};

	__forceinline Vector3 EvalGravity(const float invMass, const float dt) 
	{
		return invMass * g_gravity_vec * dt;
	}

	__forceinline Vector3 __vectorcall EvalT1PositionDelta(
		const Vector3& t0_vel, const Vector3& t0_force, const float dt
	)
	{
		return (t0_vel * dt) + (t0_force * (dt * dt * 0.5f));
	}

	__forceinline Vector3 __vectorcall EvalT1Velocity(
		const Vector3& t0_vel, const Vector3& t0_force, const Vector3& t1_force, const float dt
	)
	{
		return t0_vel + ((t0_force + t1_force) * (0.5f * dt));
	}
}


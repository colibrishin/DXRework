#include "pch.hpp"
#include "egPhysics.h"

#undef max

namespace Engine::Physics
{
	inline Vector3 EvalFriction(const Vector3& vel, float mu, float dt)
	{
		// @todo: gravity check and apply
		Vector3 invVel = -vel;

		invVel = (invVel * mu);

		// limit to zero
		if (vel.LengthSquared() < mu * dt)
		{
			return invVel * dt;
		}

		return invVel;
	}

	inline Vector3 GetActivePolarity(const Vector3& vel)
	{
		return {std::copysignf(vel.x, 1.f), std::copysignf(vel.y, 1.f), std::copysignf(vel.z, 1.f)};
	}

	inline Vector3 EvalDrag(const Vector3& vel, float k)
	{
		return -vel * k;
	}
}

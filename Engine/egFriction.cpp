#include "pch.hpp"
#include "egPhysics.h"
#include "egFriction.h"

#undef max

namespace Engine::Physics
{
	inline Vector3 EvalFriction(const Vector3& vel, float mu, float dt)
	{
		// @todo: gravity check and apply
		Vector3 invVel = -vel;

		invVel.y = 0.f;

		invVel = (invVel * mu);

		if (vel.Length() < mu * dt)
		{
			return invVel * dt;
		}

		return invVel;
	}

	inline Vector3 EvalDrag(const Vector3& vel, float k)
	{
		return -vel * k;
	}

	void FrictionVelocityGuard(Vector3& evaluated_velocity, const Vector3& friction)
	{
		const Vector3 Undo = evaluated_velocity - friction;

		if (!IsSamePolarity(evaluated_velocity.x, Undo.x))
		{
			evaluated_velocity.x = 0.0f;
		}
		if (!IsSamePolarity(evaluated_velocity.y, Undo.y))
		{
			evaluated_velocity.y = 0.0f;
		}
		if (!IsSamePolarity(evaluated_velocity.z, Undo.z))
		{
			evaluated_velocity.z = 0.0f;
		}
	}
}

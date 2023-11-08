#include "pch.hpp"
#include "egPhysics.h"

namespace Engine::Physics
{
	inline Vector3 EvalVerlet(const Vector3& vel, const Vector3& acc, float dt)
	{
		return vel + (acc * dt);
	}

	inline Vector3 EvalAngular(const Vector3& ang_vel, const Vector3& torque, float dt)
	{
		return ang_vel + (torque * dt);
	}

	inline Vector3 EvalGravity(float mass, float dt)
	{
		return 0.5f * mass * g_gravity_vec * dt;
	}
}
#include "pch.h"
#include "egKinetic.h"

namespace Engine::Physics
{
	Vector3 __vectorcall EvalVerlet(const Vector3& vel, const Vector3& acc, float dt)
	{
		return vel + (acc * dt);
	}

	Vector3 __vectorcall EvalAngular(const Vector3& ang_vel, const Vector3& torque, float dt)
	{
		return ang_vel + (torque * dt);
	}
} // namespace Engine::Physics

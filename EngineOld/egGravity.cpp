#include "pch.h"

#include "egPhysics.hpp"

namespace Engine::Physics
{
	Vector3 __vectorcall EvalGravity(float invMass, float dt)
	{
		return invMass * g_gravity_vec * dt;
	}
} // namespace Engine::Physics

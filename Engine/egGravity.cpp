#include "pch.hpp"

#include "egCollider.hpp"

namespace Engine::Physics
{
	inline Vector3 EvalGravity(float invMass, float dt)
	{
		return invMass * g_gravity_vec * dt;
	}
}

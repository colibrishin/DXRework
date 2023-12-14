#include "pch.hpp"

#include "egCollider.hpp"
#include "egPhysics.h"

namespace Engine::Physics
{
    inline Vector3 EvalGravity(float invMass, float dt)
    {
        return invMass * g_gravity_vec * dt;
    }
} // namespace Engine::Physics

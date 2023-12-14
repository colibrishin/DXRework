#pragma once

namespace Engine::Physics
{
    Vector3 EvalFriction(const Vector3& vel, float mu, float dt);
    Vector3 EvalDrag(const Vector3& vel, float k);
    void    FrictionVelocityGuard(
        Vector3&       evaluated_velocity,
        const Vector3& friction);
} // namespace Engine::Physics

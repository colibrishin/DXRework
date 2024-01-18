#pragma once

namespace Engine::Physics
{
  Vector3 __vectorcall EvalFriction(const Vector3& vel, float mu, float dt);
  Vector3 __vectorcall EvalDrag(const Vector3& vel, float k);
  void __vectorcall    FrictionVelocityGuard(
    Vector3&       evaluated_velocity,
    const Vector3& friction
  );
} // namespace Engine::Physics

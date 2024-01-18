#pragma once

namespace Engine::Physics
{
  Vector3 __vectorcall EvalVerlet(const Vector3& vel, const Vector3& acc, float dt);
  Vector3 __vectorcall EvalAngular(const Vector3& angular, const Vector3& torque, float dt);
} // namespace Engine::Physics

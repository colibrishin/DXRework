#pragma once
#include <SimpleMath.h>
#include <DirectXCollision.h>

#include "egRigidbody.hpp"

namespace Engine::Physics
{
	using namespace DirectX::SimpleMath;

	constexpr float g_gravity_acc = 9.81f;
	constexpr Vector3 g_gravity_vec = Vector3(0.0f, -g_gravity_acc, 0.0f);

	extern Vector3 EvalVerlet(const Vector3& vel, const Vector3& acc, float dt);
	extern Vector3 EvalFriction(const Vector3& vel, float mu, float dt);

	extern Vector3 GetActivePolarity(const Vector3& vel);

	extern Vector3 EvalDrag(const Vector3& vel, float k);
	extern Vector3 EvalGravity(float mass, float dt);

	extern float GetCollisionPenetrationDepth(const DirectX::BoundingOrientedBox& box, 
												const DirectX::BoundingSphere& sphere, Vector3& normal);
	extern float GetCollisionPenetrationDepth(const DirectX::BoundingSphere& sphere1, const DirectX::BoundingSphere& sphere2, Vector3& normal);
	extern float GetCollisionPenetrationDepth(const DirectX::BoundingOrientedBox& box1,
											const DirectX::BoundingOrientedBox& box2, Vector3& normal);


	extern void EvalImpulse(Vector3& pos1, Vector3& pos2, const Vector3& point, float penetration,
							const Vector3& normal,
							float invm1, float invm2, const Vector3& rv1, const Vector3& rv2, const Vector3& vel1,
							const Vector3& vel2, const DirectX::XMFLOAT3X3& inertiaT1,
							const DirectX::XMFLOAT3X3& inertiaT2, Vector3& linear1, Vector3& linear2,
							Vector3& angular1, Vector3& angular2);
	extern Vector3 EvalAngular(const Vector3& angular, const Vector3& torque, float dt);
}

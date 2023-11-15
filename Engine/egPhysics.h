#pragma once
#include <SimpleMath.h>
#include <DirectXCollision.h>

#include "egRigidbody.hpp"

namespace Engine::Physics
{
	using namespace DirectX::SimpleMath;

	constexpr float g_gravity_acc = 9.81f;
	constexpr Vector3 g_gravity_vec = Vector3(0.0f, -g_gravity_acc, 0.0f);
	constexpr float g_restitution_coefficient = 0.5f;
	constexpr float g_drag_coefficient = 0.47f;
	constexpr float g_floating_epsilon = 0.000001f;

	extern Vector3 EvalVerlet(const Vector3& vel, const Vector3& acc, float dt);
	extern Vector3 EvalFriction(const Vector3& vel, float mu, float dt);
	extern Vector3 EvalDrag(const Vector3& vel, float k);
	extern Vector3 EvalGravity(float invMass, float dt);

	extern float GetCollisionPenetrationDepth(const DirectX::BoundingOrientedBox& box, 
												const DirectX::BoundingSphere& sphere, Vector3& normal);
	extern float GetCollisionPenetrationDepth(const DirectX::BoundingSphere& sphere1, const DirectX::BoundingSphere& sphere2, Vector3& normal);
	extern float GetCollisionPenetrationDepth(const DirectX::BoundingOrientedBox& box1, const DirectX::BoundingOrientedBox& box2, Vector3& normal);

	extern Vector3 EvalAngular(const Vector3& angular, const Vector3& torque, float dt);
	
	extern Vector3 GetActivePolarity(const Vector3& vel);

	extern bool TestRayOBBIntersection(const Vector3& ray_origin, const Vector3& ray_direction, const Vector3& aabb_min,
										const Vector3& aabb_max, const Matrix& ModelMatrix,
										float& intersection_distance);

	extern Matrix CreateWorldMatrix(const Vector3& position, const Quaternion& rotation, const Vector3& scale);

	extern bool CheckGrounded(Abstract::Object& lhs, Abstract::Object& rhs);
	extern void ResolveCollision(Abstract::Object& lhs, Abstract::Object& rhs);
}

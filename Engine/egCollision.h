#pragma once

namespace Engine::Physics
{
	bool GJKAlgorithm(const Component::Collider& lhs, const Component::Collider& rhs, const Vector3& dir, Vector3& normal, float& penetration);
	Vector3 GetFurthestPoint(const std::vector<const Vector3*>& points, const Matrix& world, const Vector3& dir);
	void ResolveCollision(Abstract::Object& lhs, Abstract::Object& rhs);
}
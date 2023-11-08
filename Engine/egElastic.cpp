#include "pch.hpp"
#include "egPhysics.h"

#undef max

namespace Engine::Physics
{
	Vector3 operator*(const XMFLOAT3X3& lhs, const Vector3& rhs)
	{
		return {
			lhs._11 * rhs.x + lhs._12 * rhs.y + lhs._13 * rhs.z,
			lhs._21 * rhs.x + lhs._22 * rhs.y + lhs._23 * rhs.z,
			lhs._31 * rhs.x + lhs._32 * rhs.y + lhs._33 * rhs.z
		};
	}

	inline void EvalImpulse(Vector3& pos1, Vector3& pos2, const Vector3& point, float penetration, const Vector3& normal, float invm1, float invm2, const Vector3& rv1, const Vector3& rv2, const Vector3& vel1, const Vector3& vel2, const XMFLOAT3X3& inertiaT1, const XMFLOAT3X3& inertiaT2, Vector3& linear1, Vector3& linear2, Vector3& angular1, Vector3& angular2)
	{
		const float total_mass = invm1 + invm2;

		pos1 -= normal * penetration * (invm1 / total_mass);
		pos2 += normal * penetration * (invm2 / total_mass);

		const Vector3 rel1 = point - pos1;
		const Vector3 rel2 = point - pos2;

		const Vector3 angVel1 = rel1.Cross(rv1);
		const Vector3 angVel2 = rel2.Cross(rv2);

		const Vector3 sumVel1 = vel1 + angVel1;
		const Vector3 sumVel2 = vel2 + angVel2;

		const Vector3 contact_vel = sumVel1 - sumVel2;

		const float impulse_force = contact_vel.Dot(normal);

		const Vector3 inertia1 = (inertiaT1 * rel1.Cross(normal)).Cross(rel1);
		const Vector3 inertia2 = (inertiaT2 * rel2.Cross(normal)).Cross(rel2);

		const float angular_impulse = (inertia1 + inertia2).Dot(normal);
		const float restitution = 0.6666f;

		const float j = (-(1.0f + restitution) * impulse_force) / (total_mass + angular_impulse);
		const Vector3 impulse = normal * j;

		linear1 = impulse;
		linear2 = -impulse;

		angular1 = rel1.Cross(impulse);
		angular2 = rel2.Cross(-impulse);
	}

	inline float GetCollisionPenetrationDepth(const DirectX::BoundingOrientedBox& box, const DirectX::BoundingSphere& sphere, Vector3& normal)
	{
		XMVECTOR SphereCenter = XMLoadFloat3(&sphere.Center);
	    XMVECTOR SphereRadius = XMVectorReplicatePtr(&sphere.Radius);

	    XMVECTOR BoxCenter = XMLoadFloat3(&box.Center);
	    XMVECTOR BoxExtents = XMLoadFloat3(&box.Extents);
	    XMVECTOR BoxOrientation = XMLoadFloat4(&box.Orientation);

	    assert(DirectX::Internal::XMQuaternionIsUnit(BoxOrientation));

	    // Transform the center of the sphere to be local to the box.
	    XMVECTOR BoxMin = -BoxExtents;
	    XMVECTOR BoxMax = +BoxExtents;
		XMVECTOR Delta = XMVector3InverseRotate(XMVectorSubtract(SphereCenter, BoxCenter), BoxOrientation);

		XMVECTOR ClosestPoint = XMVectorClamp(Delta, BoxMin, BoxMax);
		Vector3 LocalPoint = XMVectorSubtract(Delta, ClosestPoint);

		float distance = XMVectorGetX(SphereRadius) - LocalPoint.Length();
		normal = XMVector3Normalize(LocalPoint);

		return distance;
	}

	inline float GetCollisionPenetrationDepth(const DirectX::BoundingSphere& sphere1, const DirectX::BoundingSphere& sphere2, Vector3& normal)
	{
		const auto radius_sum = sphere1.Radius + sphere2.Radius;
		const auto delta = sphere1.Center - sphere2.Center;
		const auto length = delta.Length();

		normal = XMVector3Normalize(sphere1.Center - sphere2.Center);

		return radius_sum - length;
	}

	inline float GetCollisionPenetrationDepth(const DirectX::BoundingOrientedBox& box1,
											const DirectX::BoundingOrientedBox& box2, Vector3& normal)
	{
		const auto size1 = box1.Extents;
		const auto size2 = box2.Extents;

		XMVECTOR pos1 = XMLoadFloat3(&box1.Center);
		XMVECTOR pos2 = XMLoadFloat3(&box2.Center);

		pos1 = XMVector3Rotate(pos1, XMLoadFloat4(&box1.Orientation));
		pos2 = XMVector3Rotate(pos2, XMLoadFloat4(&box2.Orientation));

		const Vector3 maxA = pos1 + size1;
		const Vector3 minA = pos1 - size1;

		const Vector3 maxB = pos2 + size2;
		const Vector3 minB = pos2 - size2;

		Vector3 normals[6] = 
		{
			{ 1.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f },
			{ -1.0f, 0.0f, 0.0f },
			{ 0.0f, -1.0f, 0.0f },
			{ 0.0f, 0.0f, -1.0f }
		};

		for (auto& n : normals)
		{
			n = XMVector3Rotate(n, XMLoadFloat4(&box1.Orientation));
		}

		const float distance[6] = 
		{
			maxA.x - minB.x,
			maxA.y - minB.y,
			maxA.z - minB.z,
			maxB.x - minA.x,
			maxB.y - minA.y,
			maxB.z - minA.z
		};

		float min_penetration = std::numeric_limits<float>::max();

		for (int i = 0; i < 6; ++i)
		{
			if (distance[i] < min_penetration)
			{
				min_penetration = distance[i];
				normal = normals[i];
			}
		}

		return min_penetration;
	}
}

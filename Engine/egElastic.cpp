#include "pch.hpp"

#include "egCollider.hpp"
#include "egPhysics.h"

#undef max
#undef min

namespace Engine::Physics
{
	inline void EvalImpulse(Vector3& pos1, Vector3& pos2, const Vector3& point, float penetration, const Vector3& normal, float invm1, float invm2, const Vector3& rv1, const Vector3& rv2, const Vector3& vel1, const Vector3& vel2, const XMFLOAT3X3& inertiaT1, const XMFLOAT3X3& inertiaT2, Vector3& linear1, Vector3& linear2, Vector3& angular1, Vector3& angular2)
	{
		const float total_mass = invm1 + invm2;

		const float penetration1 = (penetration * (invm1 / total_mass));
		const float penetration2 = (penetration * (invm2 / total_mass));

		pos1 -= normal * penetration1;
		pos2 += normal * penetration2;

		const Vector3 rel1 = point - pos1;
		const Vector3 rel2 = point - pos2;

		const Vector3 angVel1 = rv1.Cross(rel1);
		const Vector3 angVel2 = rv2.Cross(rel1);

		const Vector3 sumVel1 = vel1 + angVel1;
		const Vector3 sumVel2 = vel2 + angVel2;

		const Vector3 contact_vel = sumVel2 - sumVel1;

		const float impulse_force = contact_vel.Dot(normal);

		const Vector3 inertia1 = (XMTensorCross(inertiaT1, rel1.Cross(normal)).Cross(rel1));
		const Vector3 inertia2 = (XMTensorCross(inertiaT2, rel2.Cross(normal)).Cross(rel2));

		const float angular_impulse = (inertia1 + inertia2).Dot(normal);

		const float j = (-(1.0f + g_restitution_coefficient) * impulse_force) / (total_mass + angular_impulse);
		const Vector3 impulse = normal * j;

		linear1 = -impulse;
		linear2 = impulse;

		angular1 = rel1.Cross(-impulse);
		angular2 = rel2.Cross(impulse);
	}

	inline float GetCollisionPenetrationDepth(const DirectX::BoundingOrientedBox& box, const DirectX::BoundingSphere& sphere, Vector3& normal)
	{
		const Vector3 offset = sphere.Center - box.Center;
		const Quaternion boxOrientation = Quaternion(box.Orientation);

		const Vector3 BoxExtents = box.Extents;
		
		const auto offsetRotated = XMVector3InverseRotate(offset, boxOrientation);

		const Vector3 res = XMVectorClamp(offsetRotated, -BoxExtents, BoxExtents);
		const Vector3 resRotated = XMVector3Rotate(res, boxOrientation);

		// Box wise closest point
		const auto ClosestPoint = box.Center + resRotated;
		// Sphere wise closest point
		const auto pointOffset = ClosestPoint - sphere.Center;

		const float penetration = sphere.Radius - pointOffset.Length();
		normal = XMVector3Normalize(pointOffset);

		return penetration;
	}

	inline float GetCollisionPenetrationDepth(const DirectX::BoundingSphere& sphere1, const DirectX::BoundingSphere& sphere2, Vector3& normal)
	{
		const auto radius_sum = sphere1.Radius + sphere2.Radius;
		const auto delta = sphere1.Center - sphere2.Center;
		const auto length = delta.Length();

		normal = XMVector3Normalize(sphere1.Center - sphere2.Center);

		return radius_sum - length;
	}

	inline bool TestAxis(const Vector3& ray_direction, const float aabb_min, const float aabb_max, const Vector3& axis, float& tMin, float& tMax, const Vector3 delta)
	{
		// Test intersection with the 2 planes perpendicular to the OBB's axis
		{
			const float e = axis.Dot(delta);

			if (const float f = ray_direction.Dot(axis); std::fabs(f) > g_epsilon)
			{
				float t1 = (e+aabb_min)/f; // Intersection with the "left" plane
				float t2 = (e+aabb_max)/f; // Intersection with the "right" plane
				// t1 and t2 now contain distances betwen ray origin and ray-plane intersections

				// We want t1 to represent the nearest intersection, 
				// so if it's not the case, invert t1 and t2
				if (t1 > t2)
				{
					std::swap(t1, t2);
				}

				// tMax is the nearest "far" intersection (amongst the X,Y and Z planes pairs)
				tMax = std::min(t2, tMax);
				// tMin is the farthest "near" intersection (amongst the X,Y and Z planes pairs)
				tMin = std::max(t1, tMin);

				// And here's the trick :
				// If "far" is closer than "near", then there is NO intersection.
				// See the images in the tutorials for the visual explanation.
				if (tMax < tMin) 
				{
					return false;
				}

			}
			else // Rare case : the ray is almost parallel to the planes, so they don't have any "intersection"
			{ 
				if(-e+aabb_min > 0.0f || -e+aabb_max < 0.0f) 
				{
					return false;
				}
			}
		}

		return true;
	}

	inline bool TestRayOBBIntersection(const Vector3& ray_origin, const Vector3& ray_direction, const Vector3& aabb_min,
										const Vector3& aabb_max, const Matrix& ModelMatrix,
										float& intersection_distance)
	{
		// https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_custom.cpp#L83
		
		float tMin = 0.0f;
		float tMax = FLT_MAX;

		const Vector3 world_space(ModelMatrix._41, ModelMatrix._42, ModelMatrix._43);

		const Vector3 xAxis(ModelMatrix._11, ModelMatrix._12, ModelMatrix._13);
		const Vector3 yAxis(ModelMatrix._21, ModelMatrix._22, ModelMatrix._23);
		const Vector3 zAxis(ModelMatrix._31, ModelMatrix._32, ModelMatrix._33);

		const Vector3 delta = world_space - ray_origin;
		const std::vector<std::tuple<Vector3, float, float>> list
		{
			{xAxis, aabb_min.x, aabb_max.x},
			{yAxis, aabb_min.y, aabb_max.y},
			{zAxis, aabb_min.z, aabb_max.z}
		};

		for (const auto& [axis, min, max] : list)
		{
			if (!TestAxis(ray_direction, min, max, axis, tMin, tMax, delta))
			{
				return false;
			}
		}

		intersection_distance = tMin;
		return true;
	}

	Matrix CreateWorldMatrix(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
	{
		Matrix m = Matrix::CreateWorld(Vector3::Zero, Vector3::Forward, Vector3::Up);
		return m * Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(position);
	}

	Vector3 GetCubeSupportPoint(const XMFLOAT3* points, const Vector3& dir)
	{
		float best_projection = -FLT_MAX;
		Vector3 best_projection_point = Vector3::Zero;

		for (int i = 0; i < 8; ++i)
		{
			const Vector3 point = points[i];
			const float projection = point.Dot(dir);

			if (projection > best_projection)
			{
				best_projection = projection;
				best_projection_point = points[i];
			}
		}

		return best_projection_point;
	}

	inline float GetCollisionPenetrationDepth(const DirectX::BoundingOrientedBox& box1, const DirectX::BoundingOrientedBox& box2, Vector3& normal)
	{
		float distance = 0.f;

		const Vector3 box1_size = box1.Extents;
		const Vector3 box2_size = box2.Extents;

		XMFLOAT3 points[8]{}; // 8 corners of the box

		Vector3 ray_start = Vector3::Zero;
		Vector3 ray_center = box1.Center;
		Vector3 other_extents = box1.Extents;
		Vector3 target_extents = box2.Extents;
		Matrix target_world = GenerateWorldMatrix(box2);
		(box2.Center - box1.Center).Normalize(normal);

		if (box1_size.LengthSquared() > box2_size.LengthSquared())
		{
			ray_center = box2.Center;
			other_extents = box2.Extents;
			target_extents = box1.Extents;
			target_world = GenerateWorldMatrix(box1);
			(box1.Center - box2.Center).Normalize(normal);

			box2.GetCorners(points);
			ray_start = GetCubeSupportPoint(points, normal);
		}
		else
		{
			box1.GetCorners(points);
			ray_start = GetCubeSupportPoint(points, normal);
		}

		if (!TestRayOBBIntersection(ray_start, normal, -target_extents, target_extents, target_world, distance))
		{
			return 0.f;
		}

		return distance;
	}

	void ResolveCollision(Abstract::Object& lhs, Abstract::Object& rhs)
	{
		const auto rb = lhs.GetComponent<Engine::Component::Rigidbody>().lock();
		const auto tr = lhs.GetComponent<Engine::Component::Transform>().lock();
		const auto cl = lhs.GetComponent<Engine::Component::Collider>().lock();

		const auto cl_other = rhs.GetComponent<Engine::Component::Collider>().lock();
		const auto rb_other = rhs.GetComponent<Engine::Component::Rigidbody>().lock();
		const auto tr_other = rhs.GetComponent<Engine::Component::Transform>().lock();

		if(rb && cl && rb_other && cl_other)
		{
			if (rb->IsFixed())
			{
				return;
			}

			Vector3 linear_vel;
			Vector3 angular_vel;

			Vector3 other_linear_vel;
			Vector3 other_angular_vel;

			Vector3 pos = tr->GetPosition();
			Vector3 other_pos = tr_other->GetPosition();
			const Vector3 delta = (pos - other_pos);

			Vector3 normal;
			float penetration;

			cl->GetPenetration(*cl_other, normal, penetration);
			const Vector3 point = cl->GetSupportPoint(normal);


			Physics::EvalImpulse(pos, other_pos, point, penetration, normal, cl->GetInverseMass(),
								cl_other->GetInverseMass(), rb->GetAngularMomentum(),
								rb_other->GetAngularMomentum(), rb->GetLinearMomentum(), rb_other->GetLinearMomentum(),
								cl->GetInertiaTensor(), cl_other->GetInertiaTensor(), linear_vel,
								other_linear_vel, angular_vel, other_angular_vel);

			const auto fps = GetApplication().GetFPS();
			const UINT collision_count = rb->GetCollisionCount(rhs.GetID());

			const float collision_rate = static_cast<float>(collision_count) / static_cast<float>(fps);
			float inverse_collision_rate = 1.0f - collision_rate;

			if (!std::isfinite(inverse_collision_rate))
			{
				inverse_collision_rate = 1.f;
			}

			if (!rb->IsFixed())
			{
				tr->SetPosition(pos);
				rb->AddLinearMomentum(linear_vel * inverse_collision_rate);
				rb->AddAngularMomentum(angular_vel * inverse_collision_rate);
			}

			if (!rb_other->IsFixed())
			{
				tr_other->SetPosition(other_pos);
				rb_other->AddLinearMomentum(other_linear_vel * inverse_collision_rate);
				rb_other->AddAngularMomentum(other_angular_vel * inverse_collision_rate);
			}
		}
	}
}

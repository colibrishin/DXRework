#pragma once
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine::Physics 
{
	struct RaycastExtension
	{
		static bool __vectorcall TestRayOBBIntersection(
			const Vector3& origin,
			const Vector3& dir,
			const Vector3& aabb_min,
			const Vector3& aabb_max,
			const Matrix&  world,
			float&         intersection_distance,
			const float    epsilon = 1e-03
		)
		{
			float tMin = 0.f;
			float tMax = FLT_MAX;

			const Vector3 world_position = {world._41, world._42, world._43};
			const Vector3 delta          = world_position - origin;

			const auto x = Vector3{world._11, world._12, world._13};
			const auto y = Vector3{world._21, world._22, world._23};
			const auto z = Vector3{world._31, world._32, world._33};

			const std::initializer_list<std::tuple<Vector3, float, float>> list = {
				{x, aabb_min.x, aabb_max.x},
				{y, aabb_min.y, aabb_max.y},
				{z, aabb_min.z, aabb_max.z}
			};

			for (const auto& [axis, min, max] : list)
			{
				if (!TestAxis(axis, delta, dir, min, max, tMin, tMax, epsilon))
				{
					intersection_distance = 0.f;
					return false;
				}
			}

			intersection_distance = tMin;

			if (intersection_distance < 0.0f)
			{
				intersection_distance = tMax;
			}

			return true;
        }

		static bool __vectorcall TestRaySphereIntersection(
			const Vector3& start,
			const Vector3& dir,
			const Vector3& center,
			const float    radius,
			float&         intersection_distance
		)
		{
			float t0, t1; // solutions for t if the ray intersects
			// analytic solution
			Vector3 L = start - center;
			float   a = dir.Dot(dir);
			float   b = 2 * dir.Dot(L);
			float   c = L.Dot(L) - radius;

			if (!SolveQuadratic(a, b, c, t0, t1))
			{
				intersection_distance = 0.f;
				return false;
			}

			if (t0 > t1)
			{
				std::swap(t0, t1);
			}

			if (t0 < 0)
			{
				t0 = t1; // if t0 is negative, let's use t1 instead
				if (t0 < 0)
				{
					intersection_distance = 0.f;
					return false; // both t0 and t1 are negative
				}
			}

			intersection_distance = t0;
			return true;
        }

	private:
		static inline bool __vectorcall TestAxis(
			const Vector3& axis,
			const Vector3& delta,
			const Vector3& dir,
			const float&   min,
			const float&   max,
			float&         tMin,
			float&         tMax,
			const float    epsilon = 1e-03
		)
		{
			const auto e = axis.Dot(delta);
			const auto f = dir.Dot(axis);

			if (std::abs(f) > epsilon)
			{
				float t1 = (e + min) / f;
				float t2 = (e + max) / f;

				if (t1 > t2)
				{
					std::swap(t1, t2);
				}

				if (t1 > tMin)
				{
					tMin = t1;
				}

				if (t2 < tMax)
				{
					tMax = t2;
				}

				if (tMin > tMax)
				{
					return false;
				}

				if (tMax < 0)
				{
					return false;
				}
			}
			else
				if (-e + min > 0 || -e + max < 0)
				{
					return false;
				}

			return true;
		}

		static bool __vectorcall SolveQuadratic(
			const float& a,
			const float& b,
			const float& c,
			float&       x0,
			float&       x1
		)
		{
			const float discr = b * b - 4 * a * c;
			if (discr < 0)
			{
				return false;
			}
			if (discr == 0)
			{
				x0 = x1 = -0.5f * b / a;
			}
			else
			{
				const float q =
						(b > 0) ? -0.5f * (b + sqrt(discr)) : -0.5f * (b - sqrt(discr));
				x0 = q / a;
				x1 = c / q;
			}
			if (x0 > x1)
			{
				std::swap(x0, x1);
			}

			return true;
		}
	};
}

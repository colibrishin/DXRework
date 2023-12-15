#pragma once
#include "egType.h"

namespace Engine::Physics
{
    namespace GJK
    {
        bool __vectorcall GJKAlgorithm(
            const Component::Collider& lhs,
            const Component::Collider& rhs, const Vector3& dir,
            Vector3&                   normal, float&      penetration);
        Vector3 __vectorcall GetFurthestPoint(
            const std::vector<const Vector3*>& points,
            const Matrix&                      world, const Vector3& dir);
    } // namespace GJK

    namespace Raycast
    {
        bool __vectorcall TestRayOBBIntersection(
            const Vector3& origin, const Vector3&   dir,
            const Vector3& aabb_min, const Vector3& aabb_max,
            const Matrix&  world, float&            intersection_distance);
        bool __vectorcall TestRaySphereIntersection(
            const Ray& ray, const Vector3& center,
            float      radius, float&      intersection_distance);
    } // namespace Raycast
} // namespace Engine::Physics

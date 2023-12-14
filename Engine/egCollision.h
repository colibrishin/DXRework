#pragma once
#include "egType.hpp"

namespace Engine::Physics
{
    namespace GJK
    {
        bool GJKAlgorithm(
            const Component::Collider& lhs,
            const Component::Collider& rhs, const Vector3& dir,
            Vector3&                   normal, float&      penetration);
        Vector3 GetFurthestPoint(
            const std::vector<const Vector3*>& points,
            const Matrix&                      world, const Vector3& dir);
    } // namespace GJK

    namespace Raycast
    {
        bool TestRayOBBIntersection(
            const Vector3& origin, const Vector3&   dir,
            const Vector3& aabb_min, const Vector3& aabb_max,
            const Matrix&  world, float&            intersection_distance);
        bool TestRaySphereIntersection(
            const Ray& ray, const Vector3& center,
            float      radius, float&      intersection_distance);
    } // namespace Raycast
} // namespace Engine::Physics

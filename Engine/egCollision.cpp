#include "pch.hpp"
#include "egCollision.h"
#include "egCollider.hpp"
#include "egElastic.h"
#include "egPhysics.h"

#undef min

namespace Engine::Physics
{
    namespace GJK
    {
        inline bool SameDirection(const Vector3& direction, const Vector3& ao)
        {
            return direction.Dot(ao) > 0.f;
        }

        inline Vector3 GetFurthestPoint(
            const std::vector<const Vector3*>& points,
            const Matrix&                      world, const Vector3& dir)
        {
            float   max = -FLT_MAX;
            Vector3 result;

            for (const auto& point : points)
            {
                const auto  world_position = Vector3::Transform(*point, world);
                const float dist           = world_position.Dot(dir);

                if (dist > max)
                {
                    max    = dist;
                    result = world_position;
                }
            }

            return result;
        }

        inline Vector3 GetSupportPoint(
            const std::vector<const Vector3*>& lhs,
            const std::vector<const Vector3*>& rhs,
            const Matrix&                      lw, const Matrix& rw,
            const Vector3&                     dir)
        {
            const Vector3 support1 = GetFurthestPoint(lhs, lw, dir);
            const Vector3 support2 = GetFurthestPoint(rhs, rw, -dir);

            return support1 - support2;
        }

        inline bool LineSimplex(Simplex& points, Vector3& direction)
        {
            Vector3       a = points[0];
            const Vector3 b = points[1];

            const Vector3 ab = b - a;
            const Vector3 ao = -a;

            if (SameDirection(ab, ao))
            {
                direction = ab.Cross(ao).Cross(ab);
            }
            else
            {
                points    = {a};
                direction = ao;
            }

            return false;
        }

        inline bool TriangleSimplex(Simplex& points, Vector3& direction)
        {
            Vector3 a = points[0];
            Vector3 b = points[1];
            Vector3 c = points[2];

            const Vector3 ab = b - a;
            const Vector3 ac = c - a;
            const Vector3 ao = -a;

            const Vector3 abc = ab.Cross(ac);

            if (SameDirection(abc.Cross(ac), ao))
            {
                if (SameDirection(ac, ao))
                {
                    points    = {a, c};
                    direction = ac.Cross(ao).Cross(ac);
                }
                else
                {
                    return LineSimplex(points = {a, b}, direction);
                }
            }
            else
            {
                if (SameDirection(ab.Cross(abc), ao))
                {
                    return LineSimplex(points = {a, b}, direction);
                }
                if (SameDirection(abc, ao))
                {
                    direction = abc;
                }
                else
                {
                    points    = {a, c, b};
                    direction = -abc;
                }
            }

            return false;
        }

        inline bool TetrahedronSimplex(Simplex& points, Vector3& direction)
        {
            Vector3 a = points[0];
            Vector3 b = points[1];
            Vector3 c = points[2];
            Vector3 d = points[3];

            const Vector3 ab = b - a;
            const Vector3 ac = c - a;
            const Vector3 ad = d - a;
            const Vector3 ao = -a;

            const Vector3 abc = ab.Cross(ac);
            const Vector3 acd = ac.Cross(ad);
            const Vector3 adb = ad.Cross(ab);

            if (SameDirection(abc, ao))
            {
                return TriangleSimplex(points = {a, b, c}, direction);
            }

            if (SameDirection(acd, ao))
            {
                return TriangleSimplex(points = {a, c, d}, direction);
            }

            if (SameDirection(adb, ao))
            {
                return TriangleSimplex(points = {a, d, b}, direction);
            }

            return true;
        }

        inline bool NextSimplex(Simplex& points, Vector3& direction)
        {
            switch (points.size())
            {
            case 2: return LineSimplex(points, direction);
            case 3: return TriangleSimplex(points, direction);
            case 4: return TetrahedronSimplex(points, direction);
            default: break;
            }

            return false;
        }

        using NormalDistance = Vector4;
        using Index = size_t;
        using EdgeIndex = std::pair<Index, Index>;

        std::pair<std::vector<NormalDistance>, size_t> GetFaceNormals(
            const std::vector<Vector3>& polytope,
            const std::vector<Index>&   faces)
        {
            std::vector<NormalDistance> normals;
            size_t                      minTriangle = 0;
            float                       minDistance = FLT_MAX;

            for (size_t i = 0; i < faces.size(); i += 3)
            {
                const Vector3& a = polytope[faces[i]];
                const Vector3& b = polytope[faces[i + 1]];
                const Vector3& c = polytope[faces[i + 2]];

                const Vector3 ab = b - a;
                const Vector3 ac = c - a;

                Vector3 normal = ab.Cross(ac);
                normal.Normalize();

                float distance = normal.Dot(a);

                if (distance < 0)
                {
                    normal   = -normal;
                    distance = -distance;
                }

                normals.emplace_back(
                                     NormalDistance{normal.x, normal.y, normal.z, distance});

                if (distance < minDistance)
                {
                    minDistance = distance;
                    minTriangle = i / 3;
                }
            }

            return {normals, minTriangle};
        }

        inline void EPAAlgorithm(
            const std::vector<const Vector3*>& lhs,
            const std::vector<const Vector3*>& rhs,
            const Matrix&                      lw, const Matrix& rw,
            const Simplex&                     simplex, Vector3& normal,
            float&                             penetration)
        {
            const auto                    AddIfUnique = [](
                std::vector<EdgeIndex>&   edges,
                const std::vector<Index>& faces, Index a,
                Index                     b)
            {
                const auto reverse =
                        std::ranges::find(edges, std::make_pair(faces[b], faces[a]));
                if (reverse != edges.end())
                {
                    edges.erase(reverse);
                }
                else
                {
                    edges.emplace_back(faces[a], faces[b]);
                }
            };

            std::vector<Vector3> polytope;
            for (const auto& point : simplex)
            {
                polytope.push_back(point);
            }

            std::vector<Index> faces{0, 1, 2, 0, 3, 1, 0, 2, 3, 1, 3, 2};

            auto    [normals, minFace] = GetFaceNormals(polytope, faces);
            Vector3 minNormal;
            float   minDistance = FLT_MAX;
            size_t  iteration   = 0;

            while (minDistance == FLT_MAX)
            {
                minNormal   = Vector3(normals[minFace]);
                minDistance = normals[minFace].w;

                if (iteration >= g_epa_max_iteration)
                {
                    break;
                }

                Vector3 support   = GetSupportPoint(lhs, rhs, lw, rw, minNormal);
                float   sDistance = minNormal.Dot(support);

                if (std::abs(sDistance - minDistance) <= g_epsilon)
                {
                    break;
                }

                std::vector<std::pair<size_t, size_t>> edges;

                for (size_t i = 0; i < normals.size(); ++i)
                {
                    if (SameDirection(Vector3(normals[i]), support))
                    {
                        size_t f = i * 3;

                        AddIfUnique(edges, faces, f, f + 1);
                        AddIfUnique(edges, faces, f + 1, f + 2);
                        AddIfUnique(edges, faces, f + 2, f);

                        faces[f + 2] = faces.back();
                        faces.pop_back();

                        faces[f + 1] = faces.back();
                        faces.pop_back();

                        faces[f] = faces.back();
                        faces.pop_back();

                        normals[i] = normals.back();
                        normals.pop_back();

                        i--;
                    }
                }

                // @todo: evasive fix for empty edge cases
                if (edges.empty())
                {
                    break;
                }

                std::vector<size_t> newFaces;

                for (auto& [edgeIndex1, edgeIndex2] : edges)
                {
                    newFaces.push_back(edgeIndex1);
                    newFaces.push_back(edgeIndex2);
                    newFaces.push_back(polytope.size());
                }

                polytope.push_back(support);

                auto [newNormals, newMinFace] = GetFaceNormals(polytope, newFaces);

                minDistance          = FLT_MAX;
                float oldMinDistance = FLT_MAX;

                for (size_t i = 0; i < normals.size(); ++i)
                {
                    if (normals[i].w < oldMinDistance)
                    {
                        oldMinDistance = normals[i].w;
                        minFace        = i;
                    }
                }

                if (newNormals[newMinFace].w < oldMinDistance)
                {
                    minFace = newMinFace + normals.size();
                }

                faces.insert(faces.end(), newFaces.begin(), newFaces.end());
                normals.insert(normals.end(), newNormals.begin(), newNormals.end());

                iteration++;
            }

            normal      = minNormal;
            penetration = minDistance + g_epsilon;
        }

        bool GJKAlgorithm(
            const Component::Collider& lhs,
            const Component::Collider& rhs, const Vector3& dir,
            Vector3&                   normal, float&      penetration)
        {
            const Matrix lw = lhs.GetWorldMatrix();
            const Matrix rw = rhs.GetWorldMatrix();
            const auto   lv = lhs.GetVertices();
            const auto   rv = rhs.GetVertices();

            Vector3 support = GetSupportPoint(lv, rv, lw, rw, dir);

            Simplex simplex;
            simplex.push_front(support);

            Vector3 origin_dir = -support;

            size_t iteration = 0;

            while (iteration < g_gjk_max_iteration)
            {
                iteration++;

                support = GetSupportPoint(lv, rv, lw, rw, origin_dir);

                if (support.Dot(origin_dir) <= 0)
                {
                    return false;
                }

                simplex.push_front(support);

                if (NextSimplex(simplex, origin_dir))
                {
                    EPAAlgorithm(lv, rv, lw, rw, simplex, normal, penetration);

                    return true;
                }
            }

            return false;
        }
    } // namespace GJK

    namespace Raycast
    {
        bool TestAxis(
            const Vector3& axis, const Vector3& delta, const Vector3& dir,
            const float&   min, const float&    max, float&           tMin, float& tMax)
        {
            const auto e = axis.Dot(delta);
            const auto f = dir.Dot(axis);

            if (std::abs(f) > g_epsilon)
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
            else if (-e + min > 0 || -e + max < 0)
            {
                return false;
            }

            return true;
        }

        bool TestRayOBBIntersection(
            const Vector3& origin, const Vector3&   dir,
            const Vector3& aabb_min, const Vector3& aabb_max,
            const Matrix&  world, float&            intersection_distance)
        {
            float tMin = 0.f;
            float tMax = FLT_MAX;

            const Matrix inv_world = world.Invert();

            const Vector3 local_origin = Vector3::Transform(origin, inv_world);
            const Vector3 local_dir    = Vector3::TransformNormal(dir, inv_world);

            const Vector3 world_position = {world._41, world._42, world._43};
            const Vector3 delta          = world_position - local_origin;

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
                if (!TestAxis(axis, delta, local_dir, min, max, tMin, tMax))
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

        bool SolveQuadratic(
            const float& a, const float& b, const float& c, float& x0,
            float&       x1)
        {
            const float discr = b * b - 4 * a * c;
            if (discr < 0) return false;
            if (discr == 0) x0 = x1 = -0.5f * b / a;
            else
            {
                const float q =
                        (b > 0) ? -0.5f * (b + sqrt(discr)) : -0.5f * (b - sqrt(discr));
                x0 = q / a;
                x1 = c / q;
            }
            if (x0 > x1) std::swap(x0, x1);

            return true;
        }

        bool TestRaySphereIntersection(
            const Ray& ray, const Vector3& center,
            float      radius, float&      intersection_distance)
        {
            float t0, t1; // solutions for t if the ray intersects
            // analytic solution
            Vector3 L = ray.position - center;
            float   a = ray.direction.Dot(ray.direction);
            float   b = 2 * ray.direction.Dot(L);
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
    } // namespace Raycast
} // namespace Engine::Physics

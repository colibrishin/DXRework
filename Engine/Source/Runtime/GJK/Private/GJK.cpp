#include "Source/Runtime/GJK/Public/GJK.h"

#include <oneapi/tbb/task_arena.h>

#include "Source/Runtime/Core/Components/Collider/Public/Collider.hpp"

#include "Source/Runtime/GJK/Public/Simplex.hpp"
#include "Source/Runtime/Core/VertexElement/Public/VertexElement.hpp"

namespace Engine::Physics 
{ 
    namespace GJK
	{
		inline bool __vectorcall SameDirection(const Vector3& direction, const Vector3& ao)
		{
			return direction.Dot(ao) > 0.f;
		}

		Vector3 __vectorcall GetFurthestPoint(
			const VertexCollection& points,
			const Matrix&           world,
			const Vector3&          dir
		)
		{
			float                       max = -FLT_MAX;
			Vector3                     result;
			static std::vector<Vector3> out_stream;

			if (out_stream.size() < points.size())
			{
				out_stream.resize(points.size());
			}

			XMVector3TransformCoordStream
					(
					 out_stream.data(), sizeof(Vector3),
					 reinterpret_cast<const Vector3*>(points.data()),
					 sizeof(Graphics::VertexElement),
					 points.size(), world
					);

			for (int i = 0; i < points.size(); ++i)
			{
				const float dist = out_stream[i].Dot(dir);

				if (dist > max)
				{
					max    = dist;
					result = out_stream[i];
				}
			}

			return result;
		}

		Vector3 __vectorcall GetSupportPoint(
			const VertexCollection& lhs,
			const VertexCollection& rhs,
			const Matrix&           lw,
			const Matrix&           rw,
			const Vector3&          dir
		)
		{
			const Vector3 support1 = GetFurthestPoint(lhs, lw, dir);
			const Vector3 support2 = GetFurthestPoint(rhs, rw, -dir);

			return support1 - support2;
		}

		inline bool __vectorcall LineSimplex(Simplex& points, Vector3& direction)
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

		inline bool __vectorcall TriangleSimplex(Simplex& points, Vector3& direction)
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

		inline bool __vectorcall TetrahedronSimplex(Simplex& points, Vector3& direction)
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

		bool NextSimplex(Simplex& points, Vector3& direction)
		{
			switch (points.size())
			{
			case 2:
				return LineSimplex(points, direction);
			case 3:
				return TriangleSimplex(points, direction);
			case 4:
				return TetrahedronSimplex(points, direction);
			default:
				break;
			}

			return false;
		}

		using NormalDistance = Vector4;
		using Index = size_t;
		using EdgeIndex = std::pair<Index, Index>;

		std::pair<std::vector<NormalDistance>, size_t> GetFaceNormals(
			const std::vector<Vector3>& polytope,
			const std::vector<Index>&   faces
		)
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

				normals.emplace_back
						(
						 NormalDistance{normal.x, normal.y, normal.z, distance}
						);

				if (distance < minDistance)
				{
					minDistance = distance;
					minTriangle = i / 3;
				}
			}

			return {normals, minTriangle};
		}

		void __vectorcall EPAAlgorithm(
			const VertexCollection& lhs,
			const VertexCollection& rhs,
			const Matrix&           lw,
			const Matrix&           rw,
			const Simplex&          simplex,
			Vector3&                normal,
			float&                  penetration,
			std::size_t             max_epa_iteration,
			float					epsilon = 1e-03
		)
		{
			const auto                    AddIfUnique = [](
				std::vector<EdgeIndex>&   edges,
				const std::vector<Index>& faces, Index a,
				Index                     b
			)
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

				if (iteration >= max_epa_iteration)
				{
					break;
				}

				Vector3 support   = GetSupportPoint(lhs, rhs, lw, rw, minNormal);
				float   sDistance = minNormal.Dot(support);

				if (std::abs(sDistance - minDistance) <= epsilon)
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
			penetration = minDistance;
		}

		bool __vectorcall GJKAlgorithm(
			const Matrix&           lhs_world,
			const Matrix&           rhs_world,
			const VertexCollection& lhs_vertices,
			const VertexCollection& rhs_vertices,
			const Vector3&          dir,
			Vector3&                normal,
			float&                  penetration,
			const std::size_t       max_gjk_iteration,
			const std::size_t       max_epa_iteration
		)
		{
			const Matrix& lw = lhs_world;
			const Matrix& rw = rhs_world;
			const auto&   lv = lhs_vertices;
			const auto&   rv = rhs_vertices;

			Vector3 support = GetSupportPoint(lv, rv, lw, rw, dir);

			Simplex simplex;
			simplex.push_front(support);

			Vector3 origin_dir = -support;

			size_t iteration = 0;

			while (iteration < max_gjk_iteration)
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
					EPAAlgorithm(lv, rv, lw, rw, simplex, normal, penetration, max_epa_iteration);

					return true;
				}
			}

			return false;
		}
	} // namespace GJK
} // namespace Engine::Physics

namespace Engine
{
	bool GJKExtension::GetPenetration(
		const Weak<Components::Collider>& left, 
		const Weak<Components::Collider>& right,
		Vector3& normal,
		float& depth)
	{
		if (const Strong<Components::Collider>& left_locked = left.lock())
		{
			if (const Strong<Components::Collider>& right_locked = right.lock())
			{
				auto dir = left_locked->GetWorldMatrix().Translation() - right_locked->GetWorldMatrix().Translation();
				dir.Normalize();

				return Physics::GJK::GJKAlgorithm
						(
						 left_locked->GetWorldMatrix(), right_locked->GetWorldMatrix(), 
						 left_locked->GetVertices(), right_locked->GetVertices(), dir,
						 normal, depth);
			}
		}

		return false;
	}
}


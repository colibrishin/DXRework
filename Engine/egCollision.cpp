#include "pch.hpp"
#include "egCollision.h"
#include "egPhysics.h"
#include "egCollider.hpp"
#include "egElastic.h"

#undef min

namespace Engine::Physics
{
	inline bool SameDirection(const Vector3& direction, const Vector3& ao)
	{
		return direction.Dot(ao) > 0.f;
	}

	inline Vector3 GetFurthestPoint(const std::vector<const Vector3*>& points, const Matrix& world, const Vector3& dir)
	{
		float max = -FLT_MAX;
		Vector3 result;

		for (const auto& point : points)
		{
			const auto world_position = Vector3::Transform(*point, world);
			const float dist = world_position.Dot(dir);

			if (dist > max)
			{
				max = dist;
				result = world_position;
			}
		}

		return result;
	}

	inline Vector3 GetSupportPoint(const std::vector<const Vector3*>& lhs, const std::vector<const Vector3*>& rhs, const Matrix& lw, const Matrix& rw, const Vector3& dir)
	{
		const Vector3 support1 = GetFurthestPoint(lhs, lw, dir);
		const Vector3 support2 = GetFurthestPoint(rhs, rw, -dir);

		return support1 - support2;
	}

	inline bool LineSimplex(Simplex& points, Vector3& direction)
	{
		Vector3 a = points[0];
		const Vector3 b = points[1];

		const Vector3 ab = b - a;
		const Vector3 ao = -a;

		if (SameDirection(ab, ao))
		{
			direction = ab.Cross(ao).Cross(ab);
		}
		else
		{
			points = {a};
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
				points = {a, c};
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
			else
			{
				if (SameDirection(abc, ao))
				{
					direction = abc;
				}
				else
				{
					points = {a, c, b};
					direction = -abc;
				}
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

	std::pair<std::vector<NormalDistance>, size_t> GetFaceNormals(const std::vector<Vector3>& polytope, const std::vector<Index>& faces)
	{
		std::vector<NormalDistance> normals;
		size_t minTriangle = 0;
		float minDistance = FLT_MAX;

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
				normal = -normal;
				distance = -distance;
			}

			normals.emplace_back(NormalDistance{normal.x, normal.y, normal.z, distance});

			if (distance < minDistance)
			{
				minDistance = distance;
				minTriangle = i / 3;
			}
		}

		return {normals, minTriangle};
	}

	inline void EPAAlgorithm(const std::vector<const Vector3*>& lhs, const std::vector<const Vector3*>& rhs, const Matrix& lw, const Matrix& rw, const Simplex& simplex, Vector3& normal, float& penetration)
	{
		const auto AddIfUnique = [](std::vector<EdgeIndex>& edges, const std::vector<Index>& faces, Index a, Index b)
		{
			const auto reverse = std::ranges::find(edges, std::make_pair(faces[b], faces[a]));
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

		std::vector<Index> faces
		{
			0, 1, 2,
			0, 3, 1,
			0, 2, 3,
			1, 3, 2
		};

		auto [normals, minFace] = GetFaceNormals(polytope, faces);
		Vector3 minNormal;
		float minDistance = FLT_MAX;

		while (minDistance == FLT_MAX)
		{
			minNormal = Vector3(normals[minFace]);
			minDistance = normals[minFace].w;

			Vector3 support = GetSupportPoint(lhs, rhs, lw, rw, minNormal);
			float sDistance = minNormal.Dot(support);

			if (std::abs(sDistance - minDistance) > g_epsilon)
			{
				minDistance = FLT_MAX;

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

				std::vector<size_t> newFaces;

				for (auto& [edgeIndex1, edgeIndex2] : edges)
				{
					newFaces.push_back(edgeIndex1);
					newFaces.push_back(edgeIndex2);
					newFaces.push_back(polytope.size());
				}

				polytope.push_back(support);

				auto [newNormals, newMinFace] = GetFaceNormals(polytope, newFaces);

				float oldMinDistance = FLT_MAX;

				for (size_t i = 0; i < normals.size(); ++i)
				{
					if (normals[i].w < oldMinDistance)
					{
						oldMinDistance = normals[i].w;
						minFace = i;
					}
				}

				if (newNormals[newMinFace].w < oldMinDistance)
				{
					minFace = newMinFace + normals.size();
				}

				faces.insert(faces.end(), newFaces.begin(), newFaces.end());
				normals.insert(normals.end(), newNormals.begin(), newNormals.end());
			}
		}

		normal = minNormal;
		penetration = minDistance + g_epsilon;
		return;
	}

	bool GJKAlgorithm(const Component::Collider& lhs, const Component::Collider& rhs, const Vector3& dir, Vector3& normal, float& penetration)
	{
		const Matrix lw = lhs.GetWorldMatrix();
		const Matrix rw = rhs.GetWorldMatrix();
		const auto lv = lhs.GetVertices();
		const auto rv = rhs.GetVertices();

		Vector3 support = GetSupportPoint(lv, rv, lw, rw, dir);

		Simplex simplex;
		simplex.push_front(support);

		Vector3 origin_dir = -support;

		size_t iteration = 0;

		while (iteration < g_gjk_max_iteration)
		{
			iteration++;

			support = GetSupportPoint(lv, rv, lw, rw, origin_dir);

			if(support.Dot(origin_dir) <= 0)
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

			const Vector3 delta = (other_pos - pos);
			Vector3 dir;
			delta.Normalize(dir);

			Vector3 normal;
			float penetration;

			cl->GetPenetration(*cl_other, normal, penetration);
			const Vector3 point = Physics::GetFurthestPoint(cl->GetVertices(), cl->GetWorldMatrix(), -dir);


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

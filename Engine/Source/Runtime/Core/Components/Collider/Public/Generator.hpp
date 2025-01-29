#pragma once
#define _USE_MATH_DEFINES
#include <math.h>
#include <array>
#include <gcem.hpp>

#include "TypeLibrary/Public/TypeLibrary.h"
#include "VertexElement/Public/VertexElement.h"

namespace Engine 
{
	namespace details
	{
		constexpr Vector3 Normalize(const Vector3& v)
		{
			const float magnitude = gcem::sqrt(gcem::pow(v.x, 2) + gcem::pow(v.y, 2) + gcem::pow(v.z, 2));

			if (magnitude == 0.f)
			{
				return Vector3{0.f, 0.f, 0.f};
			}

			return Vector3{v.x == 0.f ? 0.f : v.x / magnitude, v.y == 0.f ? 0.f : v.y / magnitude, v.z == 0.f ? 0.f : v.z / magnitude};
		}

		constexpr Vector3 Cross(const Vector3& lhs, const Vector3& rhs)
		{
			return Vector3
			{
				lhs.y * rhs.z - lhs.z * rhs.y,
				lhs.z * rhs.x - lhs.x * rhs.z,
				lhs.x * rhs.y - lhs.y * rhs.x
			};
		}

		constexpr Vector3 Subtract(const Vector3& lhs, const Vector3& rhs)
		{
			return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
		}

		constexpr Vector3 Add(const Vector3& lhs, const Vector3& rhs)
		{
			return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
		}

		constexpr Vector3 Multiply(const Vector3& lhs, const float s)
		{
			return {s * lhs.x, s * lhs.y, s * lhs.z};
		}
		
		constexpr void GenerateTangentBinormal(
				const Vector3& v0, const Vector3&  v1,
				const Vector3& v2, const Vector2&  uv0,
				const Vector2& uv1, const Vector2& uv2,
				Vector3&       tangent, Vector3&   binormal
			)
		{
			const Vector3 edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
			const Vector3 edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

			const Vector2 deltaUV1 = {uv1.x - uv0.x, uv2.x - uv0.x};
			const Vector2 deltaUV2 = {uv1.y - uv0.y, uv2.y - uv0.y};

			if (const float delta = (deltaUV1.x * deltaUV2.y) - (deltaUV1.y * deltaUV2.x);
				delta != 0.f)
			{
				const float denominator = 1.0f / delta;

				tangent.x = denominator * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
				tangent.y = denominator * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
				tangent.z = denominator * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

				binormal.x = denominator * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
				binormal.y = denominator * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
				binormal.z = denominator * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
			}
			else
			{
				tangent.x = tangent.y = tangent.z = 0.f;
				binormal.x = binormal.y = binormal.z = 0.f;
			}
		}
		
		constexpr void GenerateTangentBinormal(auto& vertices, const auto& indices)
		{
			struct FacePair
			{
				Graphics::VertexElement* o[3];
			};

			if (indices.size() % 3 != 0)
			{
				return;
			}

			std::vector<FacePair> faces;

			for (size_t i = 0; i < indices.size(); i += 3)
			{
				const auto& i0 = indices[i];
				const auto& i1 = indices[i + 1];
				const auto& i2 = indices[i + 2];

				FacePair face{&vertices[i0], &vertices[i1], &vertices[i2]};
				faces.push_back(face);
			}

			for (const FacePair& face : faces)
			{
				Vector3 tangent(0,0,0);
				Vector3 binormal(0,0,0);

				GenerateTangentBinormal
					(
					 face.o[0]->position, face.o[1]->position,
					 face.o[2]->position, face.o[0]->texCoord,
					 face.o[1]->texCoord, face.o[2]->texCoord,
					 tangent, binormal
					);

				face.o[0]->tangent = Vector3{face.o[0]->tangent.x + tangent.x, face.o[0]->tangent.y + tangent.y, face.o[0]->tangent.z + tangent.z};
				face.o[1]->tangent = Vector3{face.o[1]->tangent.x + tangent.x, face.o[1]->tangent.y + tangent.y, face.o[1]->tangent.z + tangent.z};
				face.o[2]->tangent = Vector3{face.o[2]->tangent.x + tangent.x, face.o[2]->tangent.y + tangent.y, face.o[2]->tangent.z + tangent.z};

				face.o[0]->binormal = Vector3{face.o[0]->binormal.x + binormal.x, face.o[0]->binormal.y + binormal.y, face.o[0]->binormal.z + binormal.z};
				face.o[1]->binormal = Vector3{face.o[1]->binormal.x + binormal.x, face.o[1]->binormal.y + binormal.y, face.o[1]->binormal.z + binormal.z};
				face.o[2]->binormal = Vector3{face.o[2]->binormal.x + binormal.x, face.o[2]->binormal.y + binormal.y, face.o[2]->binormal.z + binormal.z};
			}

			for (auto& vertex : vertices)
			{
				vertex.tangent = Normalize(vertex.tangent);
				vertex.binormal = Normalize(vertex.binormal);
			}
		}
	}
	
	template <size_t Longitutde = 16, size_t Latitudue = Longitutde * 2>
	struct SphereGenerator
	{
	public:
		static constexpr uint64_t s_vertices_count = (Latitudue + 1) * (Longitutde + 1);
		static constexpr uint64_t s_indices_count = 6 * (Latitudue * Longitutde);

	private:
		static consteval std::array<UINT, s_indices_count> GenerateIndices()
		{
			std::array<UINT, s_indices_count> indices{};

			/*
			*  Indices
			*  k1--k1+1
			*  |  / |
			*  | /  |
			*  k2--k2+1
			*/
			unsigned int k1, k2;
			size_t idx = 0;

			for (int i = 0; i < Latitudue; ++i)
			{
				k1 = i * (Longitutde + 1);
				k2 = k1 + Longitutde + 1;
				// 2 Triangles per latitude block excluding the first and last longitudes blocks
				for (int j = 0; j < Longitutde; ++j, ++k1, ++k2)
				{
					if (i != 0)
					{
						indices[idx] = k1;
						indices[idx + 1] = k2;
						indices[idx + 2] = k1 + 1;
						idx += 3;
					}

					if (i != (Latitudue - 1))
					{
						indices[idx] = k1 + 1;
						indices[idx + 1] = k2;
						indices[idx + 2] = k2 + 1;
						idx += 3;
					}
				}
			}

			return indices;
		}

		static consteval std::array<Graphics::VertexElement, s_vertices_count> GenerateSphereSmooth()
		{
			float radius = 0.5f;

			std::array<Graphics::VertexElement, s_vertices_count> collection;
			float lengthInv = 1.0f / radius;    // normal
			size_t idx = 0;

			// Generate vertices
			for (int lat = 0; lat <= Latitudue; ++lat)
			{
				float theta = lat * M_PI / Latitudue; // Polar angle
				float sinTheta = gcem::sin(theta);
				float cosTheta = gcem::cos(theta);

				for (int lon = 0; lon <= Longitutde; ++lon)
				{
					float phi = (float)lon * 2.0f * M_PI / Longitutde; // Azimuthal angle
					float sinPhi = gcem::sin(phi);
					float cosPhi = gcem::cos(phi);

					// Calculate vertex position
					float x = sinTheta * cosPhi;
					float y = cosTheta;
					float z = sinTheta * sinPhi;

					float u = (float)lon / Longitutde;
					float v = (float)lat / Latitudue;

					// normalized vertex normal
					float nx = x * lengthInv;
					float ny = y * lengthInv;
					float nz = z * lengthInv;

					// Normal is the same as position for unit sphere
					collection[idx] = Graphics::VertexElement(
						Vector3(x, y, z),
						Color(0.f, 0.f, 0.f, 1.f),
						Vector2(u, v),
						Vector3(nx, ny, nz),
						Vector3(0.f, 0.f, 0.f),
						Vector3(0.f, 0.f, 0.f),
						Graphics::VertexBoneElement());

					++idx;
				}
			}

			details::GenerateTangentBinormal(collection, GetSphereIndices());
			return collection;
		}

	public:
		static consteval std::array<UINT, s_indices_count> GetSphereIndices()
		{
			constexpr std::array<UINT, s_indices_count> indices = GenerateIndices();
			return indices;
		}

		static consteval std::array<Graphics::VertexElement, s_vertices_count> GetSphereVertices()
		{
			constexpr std::array<Graphics::VertexElement, s_vertices_count> vertices = GenerateSphereSmooth();
			return vertices;
		}

		static std::vector<UINT> GetSphereIndicesAsVector()
		{
			static constexpr auto value = GetSphereIndices();
			return std::vector(value.begin(), value.end());
		}

		static std::vector<Graphics::VertexElement> GetSphereVerticesAsVector()
		{
			static constexpr auto value = GetSphereVertices();
			return std::vector(value.begin(), value.end());
		}
	};

	typedef SphereGenerator<> DefaultSphereGenerator;

	struct CubeGenerator
	{
	private:
		static constexpr size_t faceCount = 6;

		static constexpr Vector2 texCoords[] =
		{
			{1, 0},
			{1, 1},
			{0, 1},
			{0, 0},
		};

		static constexpr Vector3 normals[] =
		{
			{0, 0, 1},
			{0, 0, -1},
			{1, 0, 0},
			{-1, 0, 0},
			{0, 1, 0},
			{0, -1, 0}
		};
	
	public:
		static consteval std::array<UINT, 36> GetCubeIndices()
		{
			std::array<UINT, 36> collection{};
			
			size_t index_count = 0;
			size_t vertex_offset = 0;
			for (size_t i = 0; i < faceCount; ++i)
			{
				collection[index_count++] = vertex_offset + 2;
				collection[index_count++] = vertex_offset + 1;
				collection[index_count++] = vertex_offset + 0;

				collection[index_count++] = vertex_offset + 3;
				collection[index_count++] = vertex_offset + 2;
				collection[index_count++] = vertex_offset + 0;

				vertex_offset += 4;
			}

			return collection;
		}

		static std::vector<UINT> GetCubeIndicesAsVector()
		{
			static constexpr auto value = GetCubeIndices();
			return std::vector(value.begin(), value.end());
		}
		
	private:
		static consteval std::array<Graphics::VertexElement, 24> GenerateCubeVertices()
		{
			std::array<Graphics::VertexElement, 24> collection;

			size_t vertex_count = 0;
			for (size_t i = 0; i < faceCount; ++i)
			{
				const Vector3 normal = normals[i];
				const Vector3 basis = i >= 4 ? Vector3{ 0.f, 0.f, 1.f } : Vector3{ 0.f, 1.f, 0.f };
				const Vector3 side1 = details::Cross(normal, basis);
				const Vector3 side2 = details::Cross(normal, side1);

				collection[vertex_count++] = Graphics::VertexElement
				(
					details::Multiply(details::Subtract(details::Subtract(normal, side1), side2), 0.5),
					Color(1.f, 0.f, 0.f, 1.f),
					texCoords[0],
					normal,
					Vector3(0, 0, 0),
					Vector3(0, 0, 0),
					Graphics::VertexBoneElement()
				);
				collection[vertex_count++] = Graphics::VertexElement
				(
					details::Multiply(details::Add(details::Subtract(normal, side1), side2), 0.5),
					Color(1.f, 0.f, 0.f, 1.f),
					texCoords[1],
					normal,
					Vector3(0, 0, 0),
					Vector3(0, 0, 0),
					Graphics::VertexBoneElement()
				);
				collection[vertex_count++] = Graphics::VertexElement
				(
					details::Multiply(details::Add(details::Add(normal, side1), side2), 0.5),
					Color(1.f, 0.f, 0.f, 1.f),
					texCoords[2],
					normal,
					Vector3(0, 0, 0),
					Vector3(0, 0, 0),
					Graphics::VertexBoneElement()
				);
				collection[vertex_count++] = Graphics::VertexElement
				(
					details::Multiply(details::Subtract(details::Add(normal, side1), side2), 0.5),
					Color(1.f, 0.f, 0.f, 1.f),
					texCoords[3],
					normal,
					Vector3(0, 0, 0),
					Vector3(0, 0, 0),
					Graphics::VertexBoneElement()
				);
			}
			
			details::GenerateTangentBinormal(collection, GetCubeIndices());
			return collection;
		}

	public:
		static consteval std::array<Graphics::VertexElement, 24> GetCubeVertices()
		{
			constexpr std::array<Graphics::VertexElement, 24> vertices = GenerateCubeVertices();
			return vertices;
		}

		static std::vector<Graphics::VertexElement> GetCubeVerticesAsVector()
		{
			static constexpr auto value = GetCubeVertices();
			return std::vector(value.begin(), value.end());
		}
	};
}
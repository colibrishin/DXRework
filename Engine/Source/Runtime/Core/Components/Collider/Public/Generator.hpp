#pragma once
#define _USE_MATH_DEFINES
#include <math.h>

#include <gcem.hpp>
#include <array>

#include "TypeLibrary/Public/TypeLibrary.h"
#include "VertexElement/Public/VertexElement.h"

namespace Engine 
{
	template <size_t Longitutde = 16, size_t Latitudue = 16>
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
			float radius = 1;

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
	};

	typedef SphereGenerator<16, 16> DefaultSphereGenerator;

	struct CubeGenerator
	{
	private:
		static constexpr Vector3 vertices[8] =
		{
			{-0.5, -0.5, -0.5},
			{0.5, -0.5, -0.5},
			{0.5, 0.5, -0.5},
			{-0.5, 0.5, -0.5},
			{-0.5, -0.5, 0.5},
			{0.5, -0.5, 0.5},
			{0.5, 0.5, 0.5},
			{-0.5, 0.5, 0.5}
		};

		static constexpr Vector2 texCoords[4] =
		{
			{0, 0},
			{1, 0},
			{1, 1},
			{0, 1}
		};

		static constexpr Vector3 normals[6] =
		{
			{0, 0, 1},
			{1, 0, 0},
			{0, 0, -1},
			{-1, 0, 0},
			{0, 1, 0},
			{0, -1, 0}
		};

		static constexpr std::array<UINT, 36> stock_indices =
		{
			0, 1, 3, 3, 1, 2,
			1, 5, 2, 2, 5, 6,
			5, 4, 6, 6, 4, 7,
			4, 0, 7, 7, 0, 3,
			3, 2, 7, 7, 2, 6,
			4, 5, 0, 0, 5, 1
		};

		static constexpr uint32_t texInds[6] = { 0, 1, 3, 3, 1, 2 };

	public:
		static consteval std::array<UINT, 36> GetCubeIndices()
		{
			return stock_indices;
		}

	private:
		static consteval std::array<Graphics::VertexElement, 8> GenerateCubeVertices()
		{
			std::array<Graphics::VertexElement, 8> collection;
			constexpr auto cube_indices = GetCubeIndices();

			for (size_t i = 0; i < 8; ++i)
			{
				collection[i] = Graphics::VertexElement
				(
					vertices[cube_indices[i]],
					{ 0.f, 0.f, 1.f, 1.f },
					texCoords[texInds[i % 4]],
					normals[cube_indices[i / 6]],
					Vector3(0.f, 0.f, 0.f),
					Vector3(0.f, 0.f, 0.f),
					Graphics::VertexBoneElement()
				);
			}

			return collection;
		}

	public:
		static consteval std::array<Graphics::VertexElement, 8> GetCubeVertices()
		{
			constexpr std::array<Graphics::VertexElement, 8> vertices = GenerateCubeVertices();
			return vertices;
		}
	};
}
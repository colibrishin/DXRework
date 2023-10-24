#include "pch.hpp"
#include "egMesh.hpp"

#include <execution>
#include <tiny_obj_loader.h>

namespace Engine::Resources
{
	void Mesh::ReadOBJFile()
	{
		tinyobj::ObjReader reader;
		tinyobj::ObjReaderConfig reader_config;
		reader_config.vertex_color = true;

		// todo: wstring is not supported?
		reader.ParseFromFile(GetPath().generic_string().c_str(), reader_config);

		const auto& attrib = reader.GetAttrib();
		const auto& shapes = reader.GetShapes();
		const auto& materials = reader.GetMaterials();

		std::mutex commit_lock;
		std::mutex iter_lock;

		std::for_each(std::execution::par, shapes.begin(), shapes.end(), [&](const tinyobj::shape_t& shape)
		{
			size_t index_offset = 0;
			Resources::Shape new_shape;
			Resources::IndexCollection new_indices;

			std::mutex shape_lock;
			std::mutex index_lock;

			std::ranges::for_each(shape.mesh.num_face_vertices, [&](size_t f)
			{
				const size_t fv = size_t(shape.mesh.num_face_vertices[f]);

				for (size_t v = 0; v < fv; v++)
				{
					VertexElement vertex;

					// access to vertex
					const tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
					new_indices.push_back(index_offset + v);

					const tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
					const tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
					const tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

					vertex.position = Vector3(vx, vy, vz);

					if (idx.normal_index >= 0)
					{
						const tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
						const tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
						const tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

						vertex.normal = Vector3(nx, ny, nz);
					}

					if (idx.texcoord_index >= 0)
					{
						const tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) +
							0];
						const tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) +
							1];

						vertex.texCoord = Vector2(tx, ty);
					}

					// Optional: vertex colors
					tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
					tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
					tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];

					vertex.color = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
					new_shape.push_back(vertex);
				}

				index_offset += fv;							
			});

			{
				std::lock_guard cl(commit_lock);
				m_vertices_.push_back(new_shape);
				m_indices_.push_back(new_indices);
			}

		});
	}

	void Mesh::GenerateTangentBinormal(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector2& uv0,
		const Vector2& uv1, const Vector2& uv2, Vector3& tangent, Vector3& binormal)
	{
		const Vector3 edge1 = v1 - v0;
		const Vector3 edge2 = v2 - v0;

		const Vector2 deltaUV1 = uv1 - uv0;
		const Vector2 deltaUV2 = uv2 - uv0;

		const float delta = (deltaUV1.x * deltaUV2.y) - (deltaUV1.y * deltaUV2.x);
		const float denominator = 1.0f / delta;

		tangent.x = denominator * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent.y = denominator * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent.z = denominator * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

		binormal.x = denominator * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
		binormal.y = denominator * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
		binormal.z = denominator * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

		tangent.Normalize();
		binormal.Normalize();
	}

	void Mesh::UpdateTangentBinormal()
	{
		for (auto& shape : m_vertices_) 
		{
			std::vector<int> index;
			std::mutex shape_lock;
			const size_t face = shape.size() / 3;

			std::generate_n(std::back_inserter(index), face, [i = 0]() mutable { return i++; });
			std::ranges::for_each(index, [](int& i)
			{
				i *= 3;
			});

			std::for_each(std::execution::par, index.begin(), index.end(), [&](const int& i)
			{
				const Vector3& v0 = shape[i].position;
				const Vector3& v1 = shape[i + 1].position;
				const Vector3& v2 = shape[i + 2].position;

				const Vector2& uv0 = shape[i].texCoord;
				const Vector2& uv1 = shape[i + 1].texCoord;
				const Vector2& uv2 = shape[i + 2].texCoord;

				Vector3 tangent;
				Vector3 binormal;

				GenerateTangentBinormal(v0, v1, v2, uv0, uv1, uv2, tangent, binormal);
				
				{
					std::lock_guard sl(shape_lock);
					shape[i].tangent = tangent;
					shape[i + 1].tangent = tangent;
					shape[i + 2].tangent = tangent;

					shape[i].binormal = binormal;
					shape[i + 1].binormal = binormal;
					shape[i + 2].binormal = binormal;
				}
			});
		}
	}
}

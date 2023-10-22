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
}

#include "pch.hpp"
#include "egCubeMesh.hpp"

SERIALIZER_ACCESS_IMPL(
	Engine::Mesh::CubeMesh,
	_ARTAG(_BSTSUPER(Mesh)));

namespace Engine::Mesh
{
	CubeMesh::CubeMesh() : Mesh("")
	{
		CubeMesh::Initialize();
	}

	void CubeMesh::PreUpdate(const float& dt)
	{
	}

	void CubeMesh::Update(const float& dt)
	{
	}

	void CubeMesh::PreRender(const float& dt)
	{
	}

	void CubeMesh::Load_CUSTOM()
	{
		GeometricPrimitive::VertexCollection vertices;
		GeometricPrimitive::IndexCollection indices;
		GeometricPrimitive::CreateCube(vertices, indices, 1.f, false);

		m_vertices_.resize(1);
		m_indices_.resize(1);

		for (const auto vertex : vertices)
		{
			m_vertices_[0].push_back(Engine::VertexElement{ vertex.position,  {1.0f, 0.0f, 0.0f, 1.0f}, vertex.textureCoordinate, vertex.normal });
		}

		for (const auto index : indices)
		{
			m_indices_[0].push_back(static_cast<UINT>(index));
		}
	}

	void CubeMesh::Initialize()
	{
		Mesh::Initialize();
	}

	void CubeMesh::FixedUpdate(const float& dt)
	{
	}
}

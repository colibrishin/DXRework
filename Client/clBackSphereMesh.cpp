#include "pch.h"
#include "clBackSphereMesh.hpp"

SERIALIZER_ACCESS_IMPL(Client::Mesh::BackSphereMesh,
	_ARTAG(_BSTSUPER(Mesh)))

namespace Client::Mesh
{
	inline BackSphereMesh::BackSphereMesh() : Mesh("")
	{
		BackSphereMesh::Initialize();
	}

	inline void BackSphereMesh::PreUpdate(const float& dt)
	{
	}

	inline void BackSphereMesh::Update(const float& dt)
	{
	}

	inline void BackSphereMesh::PreRender(const float dt)
	{
	}

	inline void BackSphereMesh::Load_CUSTOM()
	{
		GeometricPrimitive::VertexCollection vertices;
		GeometricPrimitive::IndexCollection indices;
		GeometricPrimitive::CreateSphere(vertices, indices, 1.f, 16, false);

		m_vertices_.resize(1);
		m_indices_.resize(1);

		for (const auto& vertex : vertices)
		{
			m_vertices_[0].push_back(Engine::VertexElement{ vertex.position, vertex.normal, {1.0f, 0.0f, 0.0f, 1.0f}, vertex.textureCoordinate });
		}

		for (const auto index : indices)
		{
			m_indices_[0].push_back(static_cast<UINT>(index));
		}

		std::ranges::reverse(m_indices_[0]);
	}

	inline void BackSphereMesh::Initialize()
	{
		Mesh::Initialize();
	}

	inline void BackSphereMesh::FixedUpdate(const float& dt)
	{
	}
}

#pragma once
#include "../Engine/egMesh.hpp"

#include "GeometricPrimitive.h"

namespace Client::Mesh
{
	class BackSphereMesh final : public Engine::Resources::Mesh
	{
	public:
		BackSphereMesh();
		~BackSphereMesh() override = default;

		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Load_INTERNAL() override;
		void Initialize() override;
		void FixedUpdate() override;

	private:

	};

	inline BackSphereMesh::BackSphereMesh() : Mesh("")
	{
		BackSphereMesh::Initialize();
	}

	inline void BackSphereMesh::PreUpdate()
	{
	}

	inline void BackSphereMesh::Update()
	{
	}

	inline void BackSphereMesh::PreRender()
	{
	}

	inline void BackSphereMesh::Load_INTERNAL()
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

	inline void BackSphereMesh::FixedUpdate()
	{
	}
}

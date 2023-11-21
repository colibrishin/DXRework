#pragma once
#include "egMesh.hpp"
#include "GeometricPrimitive.h"

namespace Engine::Mesh
{
	class CubeMesh final : public Resources::Mesh
	{
	public:
		CubeMesh();
		~CubeMesh() override = default;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Load_INTERNAL() override;
		void Initialize() override;
		void FixedUpdate(const float& dt) override;

	private:

	};

	inline CubeMesh::CubeMesh() : Mesh("")
	{
		CubeMesh::Initialize();
	}

	inline void CubeMesh::PreUpdate(const float& dt)
	{
	}

	inline void CubeMesh::Update(const float& dt)
	{
	}

	inline void CubeMesh::PreRender(const float dt)
	{
	}

	inline void CubeMesh::Load_INTERNAL()
	{
		GeometricPrimitive::VertexCollection vertices;
		GeometricPrimitive::IndexCollection indices;
		GeometricPrimitive::CreateCube(vertices, indices, 1.f, false);

		m_vertices_.resize(1);
		m_indices_.resize(1);

		for (const auto vertex : vertices)
		{
			m_vertices_[0].push_back(Engine::VertexElement{ vertex.position, vertex.normal, {1.0f, 0.0f, 0.0f, 1.0f}, vertex.textureCoordinate });
		}

		for (const auto index : indices)
		{
			m_indices_[0].push_back(static_cast<UINT>(index));
		}
	}

	inline void CubeMesh::Initialize()
	{
		Mesh::Initialize();
	}

	inline void CubeMesh::FixedUpdate(const float& dt)
	{
	}
}

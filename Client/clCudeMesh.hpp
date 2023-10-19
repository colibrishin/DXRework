#pragma once
#include "../Engine/egMesh.hpp"
#include "GeometricPrimitive.h"

namespace Client::Mesh
{
	class CubeMesh final : public Engine::Resources::Mesh
	{
	public:
		CubeMesh();
		~CubeMesh() override = default;

		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Load_INTERNAL() override;
		void Initialize() override;

	private:

	};

	inline CubeMesh::CubeMesh() : Mesh("")
	{
		CubeMesh::Initialize();
	}

	inline void CubeMesh::PreUpdate()
	{
	}

	inline void CubeMesh::Update()
	{
	}

	inline void CubeMesh::PreRender()
	{
	}

	inline void CubeMesh::Load_INTERNAL()
	{
		GeometricPrimitive::VertexCollection vertices;
		GeometricPrimitive::IndexCollection indices;
		GeometricPrimitive::CreateCube(vertices, indices, 1.f, false);

		for (const auto vertex : vertices)
		{
			m_vertices_.push_back(Engine::VertexElement{ vertex.position, vertex.normal, {1.0f, 0.0f, 0.0f, 1.0f}, vertex.textureCoordinate });
		}

		for (const auto index : indices)
		{
			m_indices_.push_back(static_cast<UINT>(index));
		}

		Mesh::Load_INTERNAL();
	}

	inline void CubeMesh::Initialize()
	{
		Mesh::Initialize();
	}
}

#pragma once
#include <filesystem>
#include <string>
#include <SimpleMath.h>
#include "../Engine/egMesh.hpp"

namespace Client::Mesh
{
	class TriangleMesh : public Engine::Resources::Mesh
	{
	public:
		TriangleMesh();
		~TriangleMesh() override = default;

		void Initialize() override;
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

		void Load() override;
	};

	inline TriangleMesh::TriangleMesh() : Mesh("")
	{
		TriangleMesh::Initialize();
	}

	inline void TriangleMesh::Initialize()
	{
		Mesh::Initialize();
	}

	inline void TriangleMesh::PreUpdate()
	{
	}

	inline void TriangleMesh::Update()
	{
	}

	inline void TriangleMesh::PreRender()
	{
	}

	inline void TriangleMesh::Render()
	{
		Mesh::Render();
	}

	inline void TriangleMesh::Load()
	{
		m_vertices_.emplace_back(Engine::VertexElement{
			{-1.0f, -1.0f, 0.0f},
			{0.0f, 0.0f, -1.0f},
			{1.0f, 0.0f, 0.0f, 1.0f},
			{1.0f, 0.0f}
		});

		m_vertices_.emplace_back(Engine::VertexElement{
			{0.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, -1.0f},
			{1.0f, 0.0f, 0.0f, 1.0f},
			{0.5f, 1.0f}
		});

		m_vertices_.emplace_back(Engine::VertexElement{
			{1.0f, -1.0f, 0.0f},
			{0.0f, 0.0f, -1.0f},
			{1.0f, 0.0f, 0.0f, 1.0f},
			{1.0f, 1.0f}
		});

		m_indices_.emplace_back(0);
		m_indices_.emplace_back(1);
		m_indices_.emplace_back(2);
	}
}

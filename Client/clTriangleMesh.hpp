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

		void Load_INTERNAL() override;
		void FixedUpdate() override;
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

	inline void TriangleMesh::Load_INTERNAL()
	{
		m_vertices_.resize(1);
		m_indices_.resize(1);

		m_vertices_[0].emplace_back(Engine::VertexElement{
			{-1.0f, -1.0f, 0.0f},
			{0.0f, 0.0f, -1.0f},
			{1.0f, 0.0f, 0.0f, 1.0f},
			{1.0f, 0.0f}
		});

		m_vertices_[0].emplace_back(Engine::VertexElement{
			{0.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, -1.0f},
			{1.0f, 0.0f, 0.0f, 1.0f},
			{0.5f, 1.0f}
		});

		m_vertices_[0].emplace_back(Engine::VertexElement{
			{1.0f, -1.0f, 0.0f},
			{0.0f, 0.0f, -1.0f},
			{1.0f, 0.0f, 0.0f, 1.0f},
			{1.0f, 1.0f}
		});

		m_indices_[0].emplace_back(0);
		m_indices_[0].emplace_back(1);
		m_indices_[0].emplace_back(2);
	}

	inline void TriangleMesh::FixedUpdate()
	{
	}
}

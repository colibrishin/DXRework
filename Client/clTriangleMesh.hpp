#pragma once
#include <SimpleMath.h>
#include <egMesh.h>
#include <filesystem>
#include <string>

namespace Client::Meshes
{
	class TriangleMesh : public Resources::Mesh
	{
	public:
		TriangleMesh();
		~TriangleMesh() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;

		void Load_CUSTOM() override;
		void FixedUpdate(const float& dt) override;
	};

	inline TriangleMesh::TriangleMesh()
		: Mesh()
	{
		TriangleMesh::Initialize();
	}

	inline void TriangleMesh::Initialize()
	{
		Mesh::Initialize();
	}

	inline void TriangleMesh::PreUpdate(const float& dt)
	{
		Mesh::PreUpdate(dt);
	}

	inline void TriangleMesh::Update(const float& dt)
	{
		Mesh::Update(dt);
	}

	inline void TriangleMesh::Load_CUSTOM()
	{
		m_vertices_.resize(1);
		m_indices_.resize(1);

		m_vertices_.emplace_back
				(
				 Graphics::VertexElement{
					 {-1.0f, -1.0f, 0.0f},
					 {1.0f, 0.0f, 0.0f, 1.0f},
					 {1.0f, 0.0f},
					 {0.0f, 0.0f, -1.0f},
				 }
				);

		m_vertices_.emplace_back
				(
				 Graphics::VertexElement{
					 {0.0f, 1.0f, 0.0f},
					 {1.0f, 0.0f, 0.0f, 1.0f},
					 {0.5f, 1.0f},
					 {0.0f, 0.0f, -1.0f},
				 }
				);

		m_vertices_.emplace_back
				(
				 Graphics::VertexElement{
					 {1.0f, -1.0f, 0.0f},
					 {1.0f, 0.0f, 0.0f, 1.0f},
					 {1.0f, 1.0f},
					 {0.0f, 0.0f, -1.0f},
				 }
				);

		m_indices_.emplace_back(0);
		m_indices_.emplace_back(1);
		m_indices_.emplace_back(2);
	}

	inline void TriangleMesh::FixedUpdate(const float& dt) {}
} // namespace Client::Mesh

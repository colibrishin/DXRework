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
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;

		void Load_CUSTOM() override;
		void FixedUpdate(const float& dt) override;
	};

	inline TriangleMesh::TriangleMesh() : Mesh("")
	{
		TriangleMesh::Initialize();
	}

	inline void TriangleMesh::Initialize()
	{
		Mesh::Initialize();
	}

	inline void TriangleMesh::PreUpdate(const float& dt)
	{
	}

	inline void TriangleMesh::Update(const float& dt)
	{
	}

	inline void TriangleMesh::PreRender(const float dt)
	{
	}

	inline void TriangleMesh::Render(const float dt)
	{
		Mesh::Render(dt);
	}

	inline void TriangleMesh::Load_CUSTOM()
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

	inline void TriangleMesh::FixedUpdate(const float& dt)
	{
	}
}

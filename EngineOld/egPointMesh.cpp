#include "pch.h"
#include "egPointMesh.h"

SERIALIZE_IMPL(Engine::Meshes::PointMesh, _ARTAG(_BSTSUPER(Mesh)))

namespace Engine::Meshes
{
	PointMesh::PointMesh()
		: Mesh()
	{
		Mesh::Initialize();
	}

	void PointMesh::PreUpdate(const float& dt) {}

	void PointMesh::Update(const float& dt) {}

	void PointMesh::Load_CUSTOM()
	{
		m_vertices_.push_back
				(
				 {
					 {0.0f, 0.0f, 0.0f},
					 {1.0f, 0.0f, 0.0f, 1.0f},
					 {0.0f, 0.0f},
					 {0.0f, 0.0f, 0.0f}
				 }
				);

		m_indices_.push_back(0);
	}

	void PointMesh::Initialize()
	{
		Mesh::Initialize();
	}

	void PointMesh::FixedUpdate(const float& dt) {}
} // namespace Engine::Mesh

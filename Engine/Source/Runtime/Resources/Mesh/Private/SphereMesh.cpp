#include "SphereMesh.h"
#include "SphereMesh.generated.h"

#include "Components/Collider/Public/Generator.hpp"

namespace Engine::Meshes
{
	SphereMesh::SphereMesh() : Mesh() {}

	void SphereMesh::Load_CUSTOM()
	{
		m_vertices_.clear();
		m_indices_.clear();

		constexpr auto sphere_vertices = DefaultSphereGenerator::GetSphereVertices();
		m_vertices_.insert(m_vertices_.end(), sphere_vertices.begin(), sphere_vertices.end());

		constexpr auto sphere_indices = DefaultSphereGenerator::GetSphereIndices();
		m_indices_.insert(m_indices_.end(), sphere_indices.begin(), sphere_indices.end());
	}

	void SphereMesh::Initialize()
	{
		Mesh::Initialize();
	}
} // namespace Engine::Mesh
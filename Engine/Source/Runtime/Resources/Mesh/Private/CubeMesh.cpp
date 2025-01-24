#include "CubeMesh.h"
#include "CubeMesh.generated.h"

#include "Components/Collider/Public/Generator.hpp"

namespace Engine::Meshes
{
	CubeMesh::CubeMesh() : Mesh() {}

	void CubeMesh::Load_CUSTOM()
	{
		m_vertices_.clear();
		m_indices_.clear();

		constexpr auto cube_vertices = CubeGenerator::GetCubeVertices();
		m_vertices_.insert(m_vertices_.end(), cube_vertices.begin(), cube_vertices.end());

		constexpr auto cube_indices = CubeGenerator::GetCubeIndices();
		m_indices_.insert(m_indices_.end(), cube_indices.begin(), cube_indices.end());
	}

	void CubeMesh::Initialize()
	{
		Mesh::Initialize();
	}
} // namespace Engine::Mesh
#include "pch.h"
#include "egCubeMesh.h"

SERIALIZE_IMPL(Engine::Meshes::CubeMesh, _ARTAG(_BSTSUPER(Mesh)))

namespace Engine::Meshes
{
  CubeMesh::CubeMesh()
    : Mesh() { CubeMesh::Initialize(); }

  void CubeMesh::PreUpdate(const float& dt) {}

  void CubeMesh::Update(const float& dt) {}

  void CubeMesh::Load_CUSTOM()
  {
    GeometricPrimitive::VertexCollection vertices;
    GeometricPrimitive::IndexCollection  indices;
    GeometricPrimitive::CreateCube(vertices, indices, 1.f, false);

    for (const auto vertex : vertices)
    {
      m_vertices_.push_back
        (
         Graphics::VertexElement{
           vertex.position,
           {1.0f, 0.0f, 0.0f, 1.0f},
           vertex.textureCoordinate,
           vertex.normal
         }
        );
    }

    for (const auto index : indices) { m_indices_.push_back(index); }
  }

  void CubeMesh::Initialize() { Mesh::Initialize(); }

  void CubeMesh::FixedUpdate(const float& dt) {}
} // namespace Engine::Mesh

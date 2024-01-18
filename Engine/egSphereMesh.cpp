#include "pch.h"
#include "egSphereMesh.h"

SERIALIZER_ACCESS_IMPL(Engine::Meshes::SphereMesh, _ARTAG(_BSTSUPER(Mesh)))

namespace Engine::Meshes
{
  inline SphereMesh::SphereMesh()
    : Mesh() { SphereMesh::Initialize(); }

  inline void SphereMesh::PreUpdate(const float& dt) {}

  inline void SphereMesh::Update(const float& dt) {}

  inline void SphereMesh::PreRender(const float& dt) {}

  void SphereMesh::PostUpdate(const float& dt) {}

  inline void SphereMesh::Load_CUSTOM()
  {
    GeometricPrimitive::VertexCollection vertices;
    GeometricPrimitive::IndexCollection  indices;
    GeometricPrimitive::CreateSphere(vertices, indices, 1.f, 16, false);

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

  inline void SphereMesh::Initialize() { Mesh::Initialize(); }

  inline void SphereMesh::FixedUpdate(const float& dt) {}
} // namespace Engine::Mesh

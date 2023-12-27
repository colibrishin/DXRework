#include "pch.h"
#include "clBackSphereMesh.hpp"

SERIALIZER_ACCESS_IMPL(Client::Meshes::BackSphereMesh, _ARTAG(_BSTSUPER(Mesh)))

namespace Client::Meshes
{
    inline BackSphereMesh::BackSphereMesh()
    : Mesh()
    {
        BackSphereMesh::Initialize();
    }

    inline void BackSphereMesh::PreUpdate(const float& dt)
    {
        Mesh::PreUpdate(dt);
    }

    inline void BackSphereMesh::Update(const float& dt)
    {
        Mesh::Update(dt);
    }

    inline void BackSphereMesh::PreRender(const float& dt)
    {
        Mesh::PreRender(dt);
    }

    inline void BackSphereMesh::Load_CUSTOM()
    {
        GeometricPrimitive::VertexCollection vertices;
        GeometricPrimitive::IndexCollection  indices;
        GeometricPrimitive::CreateSphere(vertices, indices, 1.f, 16, false);

        for (const auto& vertex : vertices)
        {
            m_vertices_.push_back(
                                     Engine::Graphics::VertexElement{
                                         vertex.position,
                                         {1.0f, 0.0f, 0.0f, 1.0f},
                                         vertex.textureCoordinate,
                                         vertex.normal
                                     });
        }

        for (const auto index : indices)
        {
            m_indices_.push_back(index);
        }

        std::ranges::reverse(m_indices_);
    }

    inline void BackSphereMesh::Initialize()
    {
        Mesh::Initialize();
    }

    inline void BackSphereMesh::FixedUpdate(const float& dt) {}
} // namespace Client::Mesh

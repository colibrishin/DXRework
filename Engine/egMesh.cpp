#include "pch.h"
#include "egMesh.h"
#include <execution>

SERIALIZE_IMPL
(
 Engine::Resources::Mesh,
 _ARTAG(_BSTSUPER(Engine::Abstract::Resource))
 _ARTAG(m_vertices_) _ARTAG(m_indices_)
 _ARTAG(m_bounding_box_)
)

namespace Engine::Resources
{
  UINT Mesh::GetIndexCount() const { return static_cast<UINT>(m_indices_.size()); }

  const VertexCollection& Mesh::GetVertexCollection() const { return m_vertices_; }

  Mesh::Mesh(const VertexCollection& shape, const IndexCollection& indices)
    : Resource("", RES_T_MESH),
      m_vertices_(shape),
      m_indices_(indices) {}

  void __vectorcall Mesh::GenerateTangentBinormal(
    const Vector3& v0, const Vector3&  v1,
    const Vector3& v2, const Vector2&  uv0,
    const Vector2& uv1, const Vector2& uv2,
    Vector3&       tangent, Vector3&   binormal
  )
  {
    const Vector3 edge1 = v1 - v0;
    const Vector3 edge2 = v2 - v0;

    const Vector2 deltaUV1 = {uv1.x - uv0.x, uv2.x - uv0.x};
    const Vector2 deltaUV2 = {uv1.y - uv0.y, uv2.y - uv0.y};

    const float delta       = (deltaUV1.x * deltaUV2.y) - (deltaUV1.y * deltaUV2.x);
    const float denominator = 1.0f / delta;

    tangent.x = denominator * (deltaUV1.y * edge1.x - deltaUV1.x * edge2.x);
    tangent.y = denominator * (deltaUV1.y * edge1.y - deltaUV1.x * edge2.y);
    tangent.z = denominator * (deltaUV1.y * edge1.z - deltaUV1.x * edge2.z);

    binormal.x = denominator * (deltaUV2.x * edge2.x - deltaUV2.y * edge1.x);
    binormal.y = denominator * (deltaUV2.x * edge2.y - deltaUV2.y * edge1.y);
    binormal.z = denominator * (deltaUV2.x * edge2.z - deltaUV2.y * edge1.z);

    tangent.Normalize();
    binormal.Normalize();
  }

  void Mesh::UpdateTangentBinormal()
  {
    struct FacePair
    {
      VertexElement* o[3];
    };

    const auto&           indices = m_indices_;
    if (indices.size() % 3 != 0)
    {
      return;
    }

    std::vector<FacePair> faces;

    for (size_t i = 0; i < indices.size(); i += 3)
    {
      const auto& i0 = indices[i];
      const auto& i1 = indices[i + 1];
      const auto& i2 = indices[i + 2];

      FacePair face = {{&m_vertices_[i0], &m_vertices_[i1], &m_vertices_[i2]}};

      faces.push_back(face);
    }

    std::mutex commit_lock;

    std::for_each
      (
       std::execution::par, faces.begin(), faces.end(),
       [&](const FacePair& face)
       {
         Vector3 tangent;
         Vector3 binormal;

         GenerateTangentBinormal
           (
            face.o[0]->position, face.o[1]->position,
            face.o[2]->position, face.o[0]->texCoord,
            face.o[1]->texCoord, face.o[2]->texCoord, tangent,
            binormal
           );

         {
           std::lock_guard cl(commit_lock);
           face.o[0]->tangent = tangent;
           face.o[1]->tangent = tangent;
           face.o[2]->tangent = tangent;

           face.o[0]->binormal = binormal;
           face.o[1]->binormal = binormal;
           face.o[2]->binormal = binormal;
         }
       }
      );
  }

  void Mesh::PreUpdate(const float& dt) {}

  void Mesh::Update(const float& dt) {}

  void Mesh::FixedUpdate(const float& dt) {}

  void Mesh::PreRender(const float& dt) {}

  void Mesh::OnDeserialized() { Resource::OnDeserialized(); }

  void Mesh::OnSerialized() {}

  void Mesh::Initialize() {}

  void Mesh::Render(const float& dt)
  {
    GetRenderPipeline().BindVertexBuffer(m_vertex_buffer_.Get());
    GetRenderPipeline().BindIndexBuffer(m_index_buffer_.Get());
  }

  void Mesh::PostRender(const float& dt)
  {
    GetRenderPipeline().UnbindVertexBuffer();
    GetRenderPipeline().UnbindIndexBuffer();
  }

  void Mesh::PostUpdate(const float& dt) {}

  BoundingBox Mesh::GetBoundingBox() const { return m_bounding_box_; }

  Mesh::Mesh()
    : Resource("", RES_T_MESH) {}

  void Mesh::Load_INTERNAL()
  {
    Load_CUSTOM();

    UpdateTangentBinormal();

    std::vector<Vector3> vertices;
    for (const auto& vertex : m_vertices_) { vertices.push_back(vertex.position); }

    BoundingBox::CreateFromPoints
      (
       m_bounding_box_, m_vertices_.size(), vertices.data(),
       sizeof(Vector3)
      );

    GetD3Device().CreateBuffer<VertexElement>
      (
       D3D11_BIND_VERTEX_BUFFER,
       static_cast<UINT>(m_vertices_.size()),
       m_vertex_buffer_.GetAddressOf(),
       m_vertices_.data()
      );

    GetD3Device().CreateBuffer<UINT>
      (
       D3D11_BIND_INDEX_BUFFER, static_cast<UINT>(m_indices_.size()),
       m_index_buffer_.GetAddressOf(), m_indices_.data()
      );
  }

  void Mesh::Load_CUSTOM() {}

  void Mesh::Unload_INTERNAL()
  {
    m_vertex_buffer_->Release();
    m_index_buffer_->Release();
  }
} // namespace Engine::Resources

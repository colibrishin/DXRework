#pragma once
#include "egCommon.hpp"
#include "egDXCommon.h"
#include "egResource.h"
#include "egResourceManager.hpp"

struct ID3D11Buffer;
template class Microsoft::WRL::ComPtr<ID3D11Buffer>;

namespace Engine::Resources
{
  using namespace Engine::Graphics;

  class Mesh : public Abstract::Resource
  {
  public:
    RESOURCE_T(RES_T_MESH)

    Mesh(const VertexCollection& shape, const IndexCollection& indices);
    ~Mesh() override = default;
    void Initialize() override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;

    BoundingBox GetBoundingBox() const;

    UINT                    GetIndexCount() const;
    const VertexCollection& GetVertexCollection() const;

    void OnDeserialized() override;
    void OnSerialized() override;

    RESOURCE_SELF_INFER_GETTER(Mesh)

  protected:
    SERIALIZE_DECL
    Mesh();

    friend class Components::Collider;

    void         Load_INTERNAL() final;
    virtual void Load_CUSTOM();
    void         Unload_INTERNAL() override;
    
    static void __vectorcall GenerateTangentBinormal(
      const Vector3& v0, const Vector3&  v1,
      const Vector3& v2, const Vector2&  uv0,
      const Vector2& uv1, const Vector2& uv2,
      Vector3&       tangent, Vector3&   binormal
    );
    void UpdateTangentBinormal();

  protected:
    VertexCollection m_vertices_;
    IndexCollection  m_indices_;
    BoundingBox      m_bounding_box_;

    ComPtr<ID3D12Resource>  m_vertex_buffer_;
    ComPtr<ID3D12Resource>  m_index_buffer_;
    D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view_;
    D3D12_INDEX_BUFFER_VIEW m_index_buffer_view_;
  };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Mesh)

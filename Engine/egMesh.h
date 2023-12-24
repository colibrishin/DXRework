#pragma once
#include "egCommon.hpp"
#include "egDXCommon.h"
#include "egResource.h"
#include "egResourceManager.hpp"

struct ID3D11Buffer;
template class Microsoft::WRL::ComPtr<ID3D11Buffer>;

namespace Engine::Resources
{
    using Shape = std::vector<VertexElement>;
    using IndexCollection = std::vector<UINT>;
    using VertexBufferCollection = std::vector<ComPtr<ID3D11Buffer>>;
    using IndexBufferCollection = std::vector<ComPtr<ID3D11Buffer>>;

    class Mesh : public Abstract::Resource
    {
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_MESH)

        Mesh(const Shape& shape, const IndexCollection& indices);
        ~Mesh() override = default;
        void Initialize() override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        BoundingBox GetBoundingBox() const;

        const std::vector<const Vector3*>& GetVertices();
        UINT                               GetIndexCount() const;

        RESOURCE_SELF_INFER_GETTER(Mesh)

    protected:
        SERIALIZER_ACCESS
        Mesh();

        friend class Components::Collider;

        void         Load_INTERNAL() final;
        virtual void Load_CUSTOM();
        void         Unload_INTERNAL() override;


        static void __vectorcall GenerateTangentBinormal(
            const Vector3& v0, const Vector3&  v1,
            const Vector3& v2, const Vector2&  uv0,
            const Vector2& uv1, const Vector2& uv2,
            Vector3&       tangent, Vector3&   binormal);
        void UpdateTangentBinormal();

    public:
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void OnDeserialized() override;

    protected:
        Shape                       m_vertices_;
        std::vector<const Vector3*> m_flatten_vertices_;
        IndexCollection             m_indices_;
        BoundingBox                 m_bounding_box_;

        ComPtr<ID3D11Buffer> m_vertex_buffer_;
        ComPtr<ID3D11Buffer> m_index_buffer_;

    };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Mesh)

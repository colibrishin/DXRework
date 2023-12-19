#pragma once

#define TINYOBJLOADER_IMPLEMENTATION

#include <SimpleMath.h>
#include <d3d11.h>
#include <filesystem>
#include <vector>
#include <wrl/client.h>

#include "egCommon.hpp"
#include "egDXCommon.h"
#include "egRenderable.h"
#include "egResource.h"

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

        Mesh(std::filesystem::path path);
        ~Mesh() override = default;
        void Initialize() override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

        const std::vector<Shape>& GetShapes();
        const std::vector<const Vector3*>& GetVertices();

        UINT     GetRenderIndex() const;
        UINT     GetRemainingRenderIndex() const;
        UINT     GetIndexCount() const;

        void ResetRenderIndex();

    protected:
        SERIALIZER_ACCESS
        Mesh();

        friend class Components::Collider;
        friend class Manager::FBXLoader;


        void         ReadMeshFile();
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
        UINT                         m_render_index_;
        std::vector<Shape>           m_vertices_;
        std::vector<const Vector3*>  m_flatten_vertices_;
        std::vector<IndexCollection> m_indices_;
        JointMap                     m_joints_;

        VertexBufferCollection m_vertex_buffers_;
        IndexBufferCollection  m_index_buffers_;

    };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Mesh)

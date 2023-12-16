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
        ~Mesh() override = default;
        void Initialize() override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void ReadOBJFile();

        const std::vector<Shape>& GetShapes();
        const std::vector<const Vector3*>& GetVertices();

        UINT GetIndexCount() const;
        TypeName GetVirtualTypeName() const final;

    protected:
        SERIALIZER_ACCESS

        friend class Component::Collider;

        Mesh(std::filesystem::path path);

        void         Load_INTERNAL() final;
        virtual void Load_CUSTOM() = 0;
        void         Unload_INTERNAL() override;

        static void __vectorcall GenerateTangentBinormal(
            const Vector3& v0, const Vector3&  v1,
            const Vector3& v2, const Vector2&  uv0,
            const Vector2& uv1, const Vector2& uv2,
            Vector3&       tangent, Vector3&   binormal);
        void UpdateTangentBinormal();

        std::vector<Shape>           m_vertices_;
        std::vector<const Vector3*>  m_flatten_vertices_;
        std::vector<IndexCollection> m_indices_;

        VertexBufferCollection m_vertex_buffers_;
        IndexBufferCollection  m_index_buffers_;

    private:
        std::filesystem::path m_path_;
    };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Mesh)

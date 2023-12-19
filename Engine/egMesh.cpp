#include "pch.h"
#include "egMesh.h"

#include <execution>
#include <tiny_obj_loader.h>
#include "egManagerHelper.hpp"

SERIALIZER_ACCESS_IMPL(
                       Engine::Resources::Mesh,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Resource)))

namespace Engine::Resources
{
    void Mesh::ReadOBJFile()
    {
        tinyobj::ObjReader       reader;
        tinyobj::ObjReaderConfig reader_config;
        reader_config.vertex_color = true;

        // todo: wstring is not supported?
        reader.ParseFromFile(GetPath().generic_string().c_str(), reader_config);

        const auto& attrib    = reader.GetAttrib();
        const auto& shapes    = reader.GetShapes();
        const auto& materials = reader.GetMaterials();

        std::mutex commit_lock;
        std::mutex iter_lock;

        std::for_each(
                      std::execution::par, shapes.begin(), shapes.end(),
                      [&](const tinyobj::shape_t& shape)
                      {
                          UINT            index_offset = 0;
                          Shape           new_shape;
                          IndexCollection new_indices;

                          std::mutex shape_lock;
                          std::mutex index_lock;

                          std::ranges::for_each(
                                                shape.mesh.num_face_vertices, [&](UINT f)
                                                {
                                                    const UINT fv = shape.mesh.num_face_vertices[f];

                                                    for (UINT v = 0; v < fv; v++)
                                                    {
                                                        VertexElement vertex;

                                                        // access to vertex
                                                        const tinyobj::index_t idx = shape.mesh.indices[
                                                            index_offset + v];
                                                        new_indices.push_back(index_offset + v);

                                                        const tinyobj::real_t vx =
                                                                attrib.vertices[
                                                                    3 * static_cast<UINT>(idx.vertex_index) + 0];
                                                        const tinyobj::real_t vy =
                                                                attrib.vertices[
                                                                    3 * static_cast<UINT>(idx.vertex_index) + 1];
                                                        const tinyobj::real_t vz =
                                                                attrib.vertices[
                                                                    3 * static_cast<UINT>(idx.vertex_index) + 2];

                                                        vertex.position = Vector3(vx, vy, vz);

                                                        if (idx.normal_index >= 0)
                                                        {
                                                            const tinyobj::real_t nx =
                                                                    attrib.normals[
                                                                        3 * static_cast<UINT>(idx.normal_index) + 0];
                                                            const tinyobj::real_t ny =
                                                                    attrib.normals[
                                                                        3 * static_cast<UINT>(idx.normal_index) + 1];
                                                            const tinyobj::real_t nz =
                                                                    attrib.normals[
                                                                        3 * static_cast<UINT>(idx.normal_index) + 2];

                                                            vertex.normal = Vector3(nx, ny, nz);
                                                        }

                                                        if (idx.texcoord_index >= 0)
                                                        {
                                                            const tinyobj::real_t tx =
                                                                    attrib.texcoords[
                                                                        2 * static_cast<UINT>(idx.texcoord_index) +
                                                                        0];
                                                            const tinyobj::real_t ty =
                                                                    attrib.texcoords[
                                                                        2 * static_cast<UINT>(idx.texcoord_index) +
                                                                        1];

                                                            vertex.texCoord = Vector2(tx, ty);
                                                        }

                                                        // Optional: vertex colors
                                                        tinyobj::real_t red =
                                                                attrib.colors[
                                                                    3 * static_cast<UINT>(idx.vertex_index) + 0];
                                                        tinyobj::real_t green =
                                                                attrib.colors[
                                                                    3 * static_cast<UINT>(idx.vertex_index) + 1];
                                                        tinyobj::real_t blue =
                                                                attrib.colors[
                                                                    3 * static_cast<UINT>(idx.vertex_index) + 2];

                                                        vertex.color = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
                                                        new_shape.push_back(vertex);
                                                    }

                                                    index_offset += fv;
                                                });

                          {
                              std::lock_guard cl(commit_lock);
                              m_vertices_.push_back(new_shape);
                              m_indices_.push_back(new_indices);
                          }
                      });
    }

    const std::vector<Shape>& Mesh::GetShapes()
    {
        return m_vertices_;
    }

    const std::vector<const Vector3*>& Mesh::GetVertices()
    {
        return m_flatten_vertices_;
    }

    UINT Mesh::GetRenderIndex() const
    {
        return m_render_index_;
    }

    UINT Mesh::GetRemainingRenderIndex() const
    {
        const auto remain = static_cast<UINT>(m_vertices_.size()) - m_render_index_;
        return remain < 0 ? 0 : remain;
    }

    UINT Mesh::GetIndexCount() const
    {
        return static_cast<UINT>(m_indices_.size());
    }

    Mesh::Mesh(std::filesystem::path path)
    : Resource(std::move(path), RES_T_MESH),
      m_render_index_(0) {}

    void __vectorcall Mesh::GenerateTangentBinormal(
        const Vector3& v0, const Vector3&  v1,
        const Vector3& v2, const Vector2&  uv0,
        const Vector2& uv1, const Vector2& uv2,
        Vector3&       tangent, Vector3&   binormal)
    {
        const Vector3 edge1 = v1 - v0;
        const Vector3 edge2 = v2 - v0;

        const Vector2 deltaUV1 = {uv1.x - uv0.x, uv2.x - uv0.x};
        const Vector2 deltaUV2 = {uv1.y - uv0.y, uv2.y - uv0.y};

        const float delta       = (deltaUV1.x * deltaUV2.y) - (deltaUV1.y * deltaUV2.x);
        const float denominator = 1.0f / delta;

        tangent.x = denominator * (deltaUV2.y * edge1.x - deltaUV2.y * edge2.x);
        tangent.y = denominator * (deltaUV2.y * edge1.y - deltaUV2.y * edge2.y);
        tangent.z = denominator * (deltaUV2.y * edge1.z - deltaUV2.y * edge2.z);

        binormal.x = denominator * (deltaUV1.x * edge2.x - deltaUV1.y * edge1.x);
        binormal.y = denominator * (deltaUV1.x * edge2.y - deltaUV1.y * edge1.y);
        binormal.z = denominator * (deltaUV1.x * edge2.z - deltaUV1.y * edge1.z);

        tangent.Normalize();
        binormal.Normalize();
    }

    void Mesh::UpdateTangentBinormal()
    {
        struct FacePair
        {
            VertexElement* o[3];
        };

        for (size_t shape_count = 0; shape_count < m_vertices_.size();
             shape_count++)
        {
            auto& shape = m_vertices_[shape_count];

            if (m_vertices_[shape_count].size() == 0)
            {
                break;
            }

            const auto&           indices = m_indices_[shape_count];
            std::vector<FacePair> faces;

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                const auto& i0 = indices[i];
                const auto& i1 = indices[i + 1];
                const auto& i2 = indices[i + 2];

                FacePair face = {{&shape[i0], &shape[i1], &shape[i2]}};

                faces.push_back(face);
            }

            std::mutex commit_lock;

            std::for_each(
                          std::execution::par, faces.begin(), faces.end(),
                          [&](const FacePair& face)
                          {
                              Vector3 tangent;
                              Vector3 binormal;

                              GenerateTangentBinormal(
                                                      face.o[0]->position, face.o[1]->position,
                                                      face.o[2]->position, face.o[0]->texCoord,
                                                      face.o[1]->texCoord, face.o[2]->texCoord, tangent,
                                                      binormal);

                              {
                                  std::lock_guard cl(commit_lock);
                                  face.o[0]->tangent = tangent;
                                  face.o[1]->tangent = tangent;
                                  face.o[2]->tangent = tangent;

                                  face.o[0]->binormal = binormal;
                                  face.o[1]->binormal = binormal;
                                  face.o[2]->binormal = binormal;
                              }
                          });
        }
    }

    void Mesh::PreUpdate(const float& dt) {}

    void Mesh::Update(const float& dt) {}

    void Mesh::FixedUpdate(const float& dt) {}

    void Mesh::PreRender(const float& dt) {}

    void Mesh::OnDeserialized()
    {
        Resource::OnDeserialized();
    }

    void Mesh::Initialize() {}

    void Mesh::Render(const float& dt)
    {
        if (m_render_index_ >= m_vertices_.size())
        {
            return;
        }

        GetRenderPipeline().BindVertexBuffer(m_vertex_buffers_[m_render_index_].Get());
        GetRenderPipeline().BindIndexBuffer(m_index_buffers_[m_render_index_].Get());
        GetRenderPipeline().DrawIndexed(static_cast<UINT>(m_indices_[m_render_index_].size()));
        m_render_index_++;
    }

    void Mesh::PostRender(const float& dt) {}

    void Mesh::ResetRenderIndex()
    {
        m_render_index_ = 0;
    }

    Mesh::Mesh()
    : Resource("", RES_T_MESH),
      m_render_index_(0) {}

    void Mesh::Load_INTERNAL()
    {
        if (!GetPath().empty())
        {
            if (GetPath().extension() == ".obj")
            {
                ReadOBJFile();
            }
        }
        else
        {
            Load_CUSTOM();
        }

        UpdateTangentBinormal();

        for (const auto& shape : m_vertices_)
        {
            for (const auto& vertex : shape)
            {
                m_flatten_vertices_.push_back(&vertex.position);
            }
        }

        m_vertex_buffers_.resize(m_vertices_.size());
        m_index_buffers_.resize(m_indices_.size());

        for (int i = 0; i < m_vertex_buffers_.size(); ++i)
        {
            GetD3Device().CreateBuffer<VertexElement>(
                                                      D3D11_BIND_VERTEX_BUFFER,
                                                      static_cast<UINT>(m_vertices_[i].size()),
                                                      m_vertex_buffers_[i].ReleaseAndGetAddressOf(),
                                                      m_vertices_[i].data());
        }

        for (int i = 0; i < m_index_buffers_.size(); ++i)
        {
            GetD3Device().CreateBuffer<UINT>(
                                             D3D11_BIND_INDEX_BUFFER, static_cast<UINT>(m_indices_[i].size()),
                                             m_index_buffers_[i].ReleaseAndGetAddressOf(), m_indices_[i].data());
        }
    }

    void Mesh::Load_CUSTOM() {}

    void Mesh::Unload_INTERNAL()
    {
        for (auto& buffer : m_vertex_buffers_)
        {
            buffer.Reset();
        }

        for (auto& buffer : m_index_buffers_)
        {
            buffer.Reset();
        }
    }
} // namespace Engine::Resources

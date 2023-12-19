#include "pch.h"
#include "egMesh.h"

#include <execution>
#include <tiny_obj_loader.h>
#include "egManagerHelper.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


SERIALIZER_ACCESS_IMPL(
                       Engine::Resources::Mesh,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Resource)))

namespace Engine::Resources
{
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

    void Mesh::ReadMeshFile()
    {
        Assimp::Importer importer;
        const aiScene*   scene = importer.ReadFile(
                                                 GetPath().string(),
                                                 aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                 aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
                                                 aiProcess_MakeLeftHanded);

        if (scene == nullptr)
        {
            throw std::runtime_error(importer.GetErrorString());
        }

        if(scene->HasMeshes())
        {
            const auto shape_count = scene->mNumMeshes;

            for (int i = 0; i < shape_count; ++i)
            {
                Resources::Shape shape;
                IndexCollection  indices;
                const auto shape_ = scene->mMeshes[i];

                const auto v_count = shape_->mNumVertices;
                const auto f_count  = shape_->mNumFaces;

                // extract vertices
                for (int j = 0; j < v_count; ++j)
                {
                    const auto vec = shape_->mVertices[j];

                    Vector2 tex_coord = {0.f, 0.f};
                    Vector3 normal_ = {0.f, 0.f, 0.f};
                    Vector3 tangent_ = {0.f, 0.f, 0.f};
                    Vector3 binormal_ = {0.f, 0.f, 0.f};

                    if (shape_->HasTextureCoords(j))
                    {
                        const auto tex = shape_->mTextureCoords[j]; // Assuming UV exists in 2D
                        tex_coord = Vector2{tex->x, tex->y};
                    }

                    if (shape_->HasNormals())
                    {
                        const auto normal = shape_->mNormals[j];
                        normal_ = Vector3{normal.x, normal.y, normal.z};
                    }

                    if (shape_->HasTangentsAndBitangents())
                    {
                        const auto tangent = shape_->mTangents[j];
                        const auto binormal = shape_->mBitangents[j];

                        tangent_ = Vector3{tangent.x, tangent.y, tangent.z};
                        binormal_ = Vector3{binormal.x, binormal.y, binormal.z};
                    }
                    
                    shape.push_back(
                                    VertexElement
                                    {
                                        {vec.x, vec.y, vec.z},
                                        {1.0f, 0.f, 0.f, 1.f},
                                        tex_coord,
                                        normal_,
                                        tangent_,
                                        binormal_
                                    });
                }

                for (int j = 0; j < f_count; ++j)
                {
                    const auto face = shape_->mFaces[j];
                    const auto indices_ = face.mNumIndices;

                    // extract indices
                    for (int k = 0; k < indices_; ++k)
                    {
                        indices.push_back(face.mIndices[k]);
                    }
                }

                m_vertices_.push_back(shape);
                m_indices_.push_back(indices);
            }
        }
        else
        {
            throw std::runtime_error("No meshes found in file");
        }

    }

    void Mesh::Load_INTERNAL()
    {
        if (!GetPath().empty())
        {
            ReadMeshFile();
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

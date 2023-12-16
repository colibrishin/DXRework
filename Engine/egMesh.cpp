#include "pch.h"
#include "egMesh.h"

#include <execution>
#include <tiny_obj_loader.h>
#include "egManagerHelper.hpp"
#include <fbxsdk.h>

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

    void Mesh::RipVertexElementFromFBX(const FbxMesh* const mesh, Shape& shape, IndexCollection& indices, int polygon_idx)
    {
        const auto poly_size = mesh->GetPolygonSize(polygon_idx);

        for (int k = 0; k < poly_size; ++k)
        {
            const auto idx = mesh->GetPolygonVertex(polygon_idx, k);

            VertexElement vertex;

            const auto control_point = mesh->GetControlPointAt(idx);

            vertex.position = Vector3(
                                      static_cast<float>(control_point[0]),
                                      static_cast<float>(control_point[1]),
                                      static_cast<float>(control_point[2]));

            FbxVector4 normal;
            auto       flag = mesh->GetPolygonVertexNormal(polygon_idx, k, normal);

            vertex.normal = Vector3(
                                    static_cast<float>(normal[0]),
                                    static_cast<float>(normal[1]),
                                    static_cast<float>(normal[2]));

            FbxVector2 uv;
            bool       unmapped_uv = false;

            FbxStringList uv_names;
            mesh->GetUVSetNames(uv_names);

            flag = mesh->GetPolygonVertexUV(polygon_idx, k, uv_names[0], uv, unmapped_uv);

            vertex.texCoord = Vector2(
                                      static_cast<float>(uv[0]),
                                      static_cast<float>(uv[1]));

            vertex.color = Vector4(1.0f, 0.0f, 0.0f, 1.0f);

            shape[idx] = vertex;
            indices.push_back(idx);
        }
    }

    void Mesh::IterateFBXMesh(FbxNode* const child)
    {
        const auto mesh = child->GetMesh();

        const auto vertex_count  = mesh->GetControlPointsCount();
        const auto polygon_count = mesh->GetPolygonCount();

        Shape           shape;
        IndexCollection indices;

        shape.resize(vertex_count);

        for (int j = 0; j < polygon_count; ++j)
        {
            RipVertexElementFromFBX(mesh, shape, indices, j);
        }

        m_vertices_.push_back(shape);
        m_indices_.push_back(indices);
    }

    void Mesh::ReadFBXFile()
    {
        const auto fbx_mgr = FbxManager::Create();
        if (!fbx_mgr)
        {
            throw std::runtime_error("Failed to create FBX manager");
        }

        const auto fbx_io_settings = FbxIOSettings::Create(fbx_mgr, IOSROOT);
        fbx_mgr->SetIOSettings(fbx_io_settings);
        const auto fbx_scene = FbxScene::Create(fbx_mgr, "Scene");

        FbxImporter* fbx_importer = FbxImporter::Create(fbx_mgr, "FBXImporter");
        fbx_importer->Initialize(GetPath().generic_string().c_str(), -1, fbx_mgr->GetIOSettings());
        fbx_importer->Import(fbx_scene);

        FbxGeometryConverter fbx_converter(fbx_mgr);
        fbx_converter.Triangulate(fbx_scene, true);

        const FbxAxisSystem this_system(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityEven, FbxAxisSystem::eLeftHanded);
        this_system.DeepConvertScene(fbx_scene);

        const auto root = fbx_scene->GetRootNode();
        const auto child_count = root->GetChildCount();

        if (child_count == 0)
        {
            throw std::runtime_error("No child node found");
        }

        for (int i = 0; i < child_count; ++i)
        {
            const auto child = root->GetChild(i);

            if (child->GetNodeAttribute() == nullptr)
            {
                continue;
            }

            const auto attr_type = child->GetNodeAttribute()->GetAttributeType();

            if (attr_type == FbxNodeAttribute::eMesh)
            {
                IterateFBXMesh(child);
            }
        }

        fbx_importer->Destroy();
        fbx_scene->Destroy();
        fbx_mgr->Destroy();
    }

    const std::vector<Shape>& Mesh::GetShapes()
    {
        return m_vertices_;
    }

    const std::vector<const Vector3*>& Mesh::GetVertices()
    {
        return m_flatten_vertices_;
    }

    UINT Mesh::GetIndexCount() const
    {
        return static_cast<UINT>(m_indices_.size());
    }

    Mesh::Mesh(std::filesystem::path path)
    : Resource(std::move(path), RESOURCE_PRIORITY_MESH) {}

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

    void Mesh::Initialize() {}

    void Mesh::Render(const float& dt)
    {
        for (int i = 0; i < m_vertex_buffers_.size(); ++i)
        {
            GetRenderPipeline().BindVertexBuffer(m_vertex_buffers_[i].Get());
            GetRenderPipeline().BindIndexBuffer(m_index_buffers_[i].Get());
            GetRenderPipeline().DrawIndexed(static_cast<UINT>(m_indices_[i].size()));
        }

        GetRenderPipeline().BindResource(SR_TEXTURE, nullptr);
        GetRenderPipeline().BindResource(SR_NORMAL_MAP, nullptr);
    }

    void Mesh::PostRender(const float& dt) {}

    TypeName Mesh::GetVirtualTypeName() const
    {
        return typeid(Mesh).name();
    }

    void Mesh::Load_INTERNAL()
    {
        if (!GetPath().empty())
        {
            if (GetPath().extension() == ".obj")
            {
                ReadOBJFile();
            }
            if (GetPath().extension() == ".fbx")
            {
                ReadFBXFile();
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

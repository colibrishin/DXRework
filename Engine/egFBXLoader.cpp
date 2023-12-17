#include "pch.h"
#include "egFBXLoader.h"

#include "egCubeMesh.h"
#include "egMesh.h"

namespace Engine::Manager
{
    FBXLoader::~FBXLoader()
    {
        m_fbx_importer_->Destroy();
        m_fbx_scene_->Destroy();
        m_fbx_manager_->Destroy();
    }

    void FBXLoader::PreUpdate(const float& dt) { }

    void FBXLoader::Update(const float& dt) { }

    void FBXLoader::FixedUpdate(const float& dt) { }

    void FBXLoader::PreRender(const float& dt) { }

    void FBXLoader::Render(const float& dt) { }

    void FBXLoader::PostRender(const float& dt) { }

    void FBXLoader::Initialize()
    {
        m_fbx_manager_ = FbxManager::Create();
        if (!m_fbx_manager_)
        {
            throw std::runtime_error("Failed to create FBX manager");
        }

        const auto fbx_io_settings = FbxIOSettings::Create(m_fbx_manager_, IOSROOT);
        m_fbx_manager_->SetIOSettings(fbx_io_settings);
        m_fbx_scene_ = FbxScene::Create(m_fbx_manager_, "Scene");

        m_fbx_importer_ = FbxImporter::Create(m_fbx_manager_, "FBXImporter");
    }

    void FBXLoader::LoadFBXFile(const std::filesystem::path& path, Resources::Mesh& target_mesh)
    {
        m_fbx_importer_->Initialize(path.generic_string().c_str(), -1, m_fbx_manager_->GetIOSettings());
        m_fbx_importer_->Import(m_fbx_scene_);

        const FbxAxisSystem this_system(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityEven, FbxAxisSystem::eLeftHanded);
        this_system.DeepConvertScene(m_fbx_scene_);
        m_fbx_scene_->GetGlobalSettings().SetAxisSystem(this_system);

        FbxGeometryConverter fbx_converter(m_fbx_manager_);
        fbx_converter.Triangulate(m_fbx_scene_, true);

        const auto root = m_fbx_scene_->GetRootNode();
        const auto root_child_count = root->GetChildCount();

        if (root_child_count == 0)
        {
            throw std::runtime_error("No child node found");
        }

        std::stack<FbxNode*> node_stack;
        std::stack<UINT>     depth_stack;
        node_stack.push(root);
        depth_stack.push(0);

        // vertex elements, and skeleton
        while (!node_stack.empty())
        {
            const auto node = node_stack.top();
            node_stack.pop();

            const auto depth = depth_stack.top();
            depth_stack.pop();

            const auto attr = node->GetNodeAttribute();

            if (attr != nullptr)
            {
                const auto attr_type = attr->GetAttributeType();

                if (attr_type == FbxNodeAttribute::eMesh)
                {
                    IterateFBXMesh(node, target_mesh);
                }

                if (attr_type == FbxNodeAttribute::eSkeleton)
                {
                    IterateFBXSkeleton(node, depth, target_mesh);
                }
            }

            const auto child_count = node->GetChildCount();

            for (int i = child_count - 1; i >= 0; --i)
            {
                node_stack.push(node->GetChild(i));
            }

            for (int i = 0; i < child_count; ++i)
            {
                depth_stack.push(depth + 1);
            }
        }

        // skinning
        node_stack.push(root);
        while (!node_stack.empty())
        {
            const auto node = node_stack.top();
            node_stack.pop();

            const auto attr = node->GetNodeAttribute();

            if (attr != nullptr)
            {
                const auto attr_type = attr->GetAttributeType();

                if (attr_type == FbxNodeAttribute::eMesh)
                {
                    IterateFBXSkin(node, target_mesh);
                }

            }

            const auto child_count = node->GetChildCount();

            for (int i = child_count - 1; i >= 0; --i)
            {
                node_stack.push(node->GetChild(i));
            }
        }
    }

    void FBXLoader::RipVertexElementFromFBX(const FbxMesh* const mesh, Resources::Shape& shape, Resources::IndexCollection& indices, int polygon_idx)
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

    void FBXLoader::IterateFBXMesh(FbxNode* const child, Engine::Resources::Mesh& target_mesh)
    {
        const auto mesh = child->GetMesh();

        const auto vertex_count  = mesh->GetControlPointsCount();
        const auto polygon_count = mesh->GetPolygonCount();

        Resources::Shape           shape;
        Resources::IndexCollection indices;

        shape.resize(vertex_count);

        for (int j = 0; j < polygon_count; ++j)
        {
            RipVertexElementFromFBX(mesh, shape, indices, j);
        }

        target_mesh.m_vertices_.push_back(shape);
        target_mesh.m_indices_.push_back(indices);
    }

    void FBXLoader::IterateFBXSkeleton(FbxNode* node, UINT depth, Engine::Resources::Mesh& target_mesh)
    {
        Joint joint;

        joint.index = static_cast<UINT>(target_mesh.m_joints_.size());
        joint.parent_index = depth;

        const auto transform = node->EvaluateGlobalTransform();

        for (int i = 0; i < 4; ++i)
        {
            const auto row = transform.GetRow(i);

            for (int j = 0; j < 4; ++j)
            {
                joint.global_transform.m[i][j] = static_cast<float>(row[j]);
            }
        }

        for (int i = 0; i < 4; ++i)
        {
            const auto row = node->EvaluateLocalTransform().GetRow(i);

            for (int j = 0; j < 4; ++j)
            {
                joint.local_transform.m[i][j] = static_cast<float>(row[j]);
            }
        }

        joint.name = node->GetName();
        target_mesh.m_joints_[node->GetName()] = joint;
    }

    void FBXLoader::IterateFBXSkin(FbxNode* child, Engine::Resources::Mesh& target_mesh)
    {
        const auto mesh = child->GetMesh();

        const auto shape_count = target_mesh.m_vertices_.size();

        for (int j = 0; j < shape_count; ++j)
        {
            RipDeformation(mesh, target_mesh.m_vertices_[j], target_mesh.m_indices_[j], target_mesh.m_joints_, j);
        }
    }

    void FBXLoader::RipDeformation(const FbxMesh* mesh, Resources::Shape& shape, const Resources::IndexCollection& vector, const JointMap& joints, int i)
    {
        const auto deformer_count = mesh->GetDeformerCount();

        for (int j = 0; j < deformer_count; ++j)
        {
            const auto deformer = mesh->GetDeformer(j);
            const auto skin = static_cast<FbxSkin*>(deformer);

            const auto cluster_count = skin->GetClusterCount();

            for (int k = 0; k < cluster_count; ++k)
            {
                const auto cluster = skin->GetCluster(k);

                const auto joint_name = cluster->GetLink()->GetName();

                const auto joint = joints.find(joint_name);

                if (joint == joints.end())
                {
                    continue;
                }

                const auto joint_index = joint->second.index;

                for (int l = 0; l < cluster->GetControlPointIndicesCount(); ++l)
                {
                    const auto control_point_index  = cluster->GetControlPointIndices()[l];
                    const auto control_point_weight = cluster->GetControlPointWeights()[l];

                    const auto vertex_index = vector[control_point_index];

                    auto& vertex = shape[vertex_index];

                    for (int m = 0; m < 4; ++m)
                    {
                        if (vertex.deformation[m].weight == 0.0f)
                        {
                            vertex.deformation[m].joint_index = joint_index;
                            vertex.deformation[m].weight      = static_cast<float>(control_point_weight);
                            break;
                        }
                    }
                }
            }
        }
    }
}

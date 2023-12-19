#include "pch.h"
#include "egFBXLoader.h"

#include "egCubeMesh.h"
#include "egMesh.h"

namespace Engine::Manager
{
    FBXLoader::~FBXLoader()
    {
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

            VertexElement vertex
            {
                Vector3::Zero,
                Vector4::Zero,
                Vector2::Zero,
                Vector3::Zero,
                Vector3::Zero,
                Vector3::Zero,
                {}
            };

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

        joint.index                            = static_cast<UINT>(target_mesh.m_joints_.size());
        joint.parent_index                     = depth;
        joint.name                             = node->GetName();
        target_mesh.m_joints_[node->GetName()] = joint;
    }

    void FBXLoader::IterateFBXSkin(FbxNode* child, Engine::Resources::Mesh& target_mesh)
    {
        const auto mesh = child->GetMesh();

        const auto shape_count = target_mesh.m_vertices_.size();

        for (int j = 0; j < shape_count; ++j)
        {
            RipDeformation(mesh, target_mesh.m_vertices_, target_mesh.m_joints_);
        }
    }

    Vector4 __vectorcall FBXLoader::FbxToVector4(const FbxVector4& v)
    {
        return {
            static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]),
            static_cast<float>(v[3])
        };
    };

    Matrix __vectorcall FBXLoader::FbxToMatrix(const FbxAMatrix& m)
    {
        return Matrix
        {
            FbxToVector4(m.GetRow(0)),
            FbxToVector4(m.GetRow(1)),
            FbxToVector4(m.GetRow(2)),
            FbxToVector4(m.GetRow(3))
        };
    };

    void FBXLoader::RipDeformation(const FbxMesh* mesh, std::vector<Resources::Shape>& shape, JointMap& joints)
    {
        const auto deformer_count = mesh->GetDeformerCount();

        for (int j = 0; j < deformer_count; ++j)
        {
            const auto deformer = mesh->GetDeformer(j);
            const auto skin = static_cast<FbxSkin*>(deformer);

            const auto geometry_transform = FbxAMatrix{
                    mesh->GetNode()->GetGeometricTranslation(FbxNode::eSourcePivot),
                    mesh->GetNode()->GetGeometricRotation(FbxNode::eSourcePivot),
                    mesh->GetNode()->GetGeometricScaling(FbxNode::eSourcePivot)
            };

            const auto cluster_count = skin->GetClusterCount();

            for (int k = 0; k < cluster_count; ++k)
            {
                const auto cluster = skin->GetCluster(k);

                const auto joint_name = cluster->GetLink()->GetName();
                auto       joint      = joints.find(joint_name);

                if (joint == joints.end())
                {
                    continue;
                }

                FbxAMatrix bindTimeMeshTransform;
                FbxAMatrix bindTimeClusterTransform;

                cluster->GetTransformMatrix(bindTimeClusterTransform);
                cluster->GetTransformLinkMatrix(bindTimeMeshTransform);

                joint->second.bind_pose_inverse = FbxToMatrix(bindTimeClusterTransform.Inverse() * bindTimeMeshTransform * geometry_transform);

                const auto joint_index = joint->second.index;
                const auto influence_count = cluster->GetControlPointIndicesCount();

                for (int l = 0; l < influence_count; ++l)
                {
                    const auto control_point_index  = cluster->GetControlPointIndices()[l];
                    const auto control_point_weight = cluster->GetControlPointWeights()[l];

                    const auto vertex_index = control_point_index;

                    auto& vertex = AccessShapeSerialized(shape, vertex_index);

                    for (int m = 0; m < 4; ++m)
                    {
                        if (vertex.deformations[m].weight == 0.0f)
                        {
                            vertex.deformations[m].joint_index = joint_index;
                            vertex.deformations[m].weight      = static_cast<float>(control_point_weight);
                            break;
                        }
                    }
                }

                const FbxAnimStack* anim_stack = m_fbx_scene_->GetSrcObject<FbxAnimStack>(0);
                FbxString           anim_name  = anim_stack->GetName();
                const FbxTakeInfo*  take_info  = m_fbx_scene_->GetTakeInfo(anim_name);

                FbxTime           start_time  = take_info->mLocalTimeSpan.GetStart();
                FbxTime           end_time    = take_info->mLocalTimeSpan.GetStop();
                const FbxLongLong anim_length = end_time.GetFrameCount(FbxTime::eFrames24) - start_time.GetFrameCount(FbxTime::eFrames24) + 1;
                joint->second.key_frames.resize(anim_length);

                for (auto t = start_time.GetFrameCount(FbxTime::eFrames24); t <= end_time.GetFrameCount(FbxTime::eFrames24); ++t)
                {
                    FbxTime time;
                    time.SetFrame(t, FbxTime::eFrames24);

                    KeyFrame kf;
                    kf.frame     = t;
                    kf.name      = anim_name;

                    const auto offset = mesh->GetNode()->EvaluateGlobalTransform() * geometry_transform;
                    const auto animation_transform = offset.Inverse() * cluster->GetLink()->EvaluateGlobalTransform(time);

                    kf.transform = FbxToMatrix(animation_transform);
                    kf.start_frame = static_cast<UINT>(start_time.GetFrameCount(FbxTime::eFrames24));
                    kf.end_frame   = static_cast<UINT>(end_time.GetFrameCount(FbxTime::eFrames24));
                    joint->second.key_frames[t] = kf;
                }
            }
        }
    }
}

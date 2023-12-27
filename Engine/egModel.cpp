#include "pch.h"
#include "egModel.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include "egResourceManager.hpp"
#include "egMesh.h"
#include "egNormalMap.h"
#include "egBaseAnimation.h"
#include "egBone.h"
#include "egBoneAnimation.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Resources::Model,
                       _ARTAG(_BSTSUPER(Resource))
                       _ARTAG(m_animations_)
                       _ARTAG(m_bone_)
                       _ARTAG(m_meshes_)
                       _ARTAG(m_normal_maps_)
                       _ARTAG(m_textures_))

namespace Engine::Resources
{
    Model::Model(const std::filesystem::path& path)
    : Resource(path, RES_T_MODEL),
      m_bounding_box_({}) {}

    void Model::PreUpdate(const float& dt) {}

    void Model::Update(const float& dt) {}

    void Model::FixedUpdate(const float& dt) {}

    void Model::PreRender(const float& dt) {}

    void Model::Render(const float& dt) {}

    void Model::PostRender(const float& dt) {}

    void Model::PostUpdate(const float& dt) {}

    BoundingBox Model::GetBoundingBox() const
    {
        return m_bounding_box_;
    }

    WeakMesh Model::GetMesh(const std::string& name) const
    {
        const auto it = std::ranges::find_if(
                                             m_meshes_
                                             , [&name](const auto& mesh)
                                             {
                                                 return mesh->GetName() == name;
                                             });

        if (it != m_meshes_.end())
        {
            return *it;
        }

        return {};
    }

    WeakMesh Model::GetMesh(const UINT index) const
    {
        if (m_meshes_.size() > index)
        {
            return m_meshes_[index];
        }

        return {};
    }

    const std::vector<const Vector3*>& Model::GetVertices() const
    {
        return m_cached_vertices_;
    }

    WeakBoneAnimation Model::GetAnimation(const std::string& name) const
    {
        const auto it = std::ranges::find_if(
                                             m_animations_
                                             , [&name](const auto& anim)
                                             {
                                                 return anim->GetName() == name;
                                             });

        if (it != m_animations_.end())
        {
            return *it;
        }

        return {};
    }

    WeakBoneAnimation Model::GetAnimation(const UINT index) const
    {
        if (m_animations_.size() > index)
        {
            return m_animations_[index];
        }

        return {};
    }

    const std::map<UINT, BoundingOrientedBox>& Model::GetBoundingBoxes() const
    {
        return m_bone_bounding_boxes_;
    }

    UINT Model::GetMeshCount() const
    {
        return m_meshes_.size();
    }

    std::vector<StrongMesh> Model::GetMeshes() const
    {
        return m_meshes_;
    }

    StrongModel           Model::Create(const std::string& name, const std::vector<StrongResource>& resources) {
        Model m("");

        if (const auto res_model = GetResourceManager().GetResource<Model>(name).lock())
        {
            return res_model;
        }

        for (auto& res : resources)
        {
            if (res->GetResourceType() == RES_T_MESH)
            {
                m.m_meshes_.push_back(boost::static_pointer_cast<Mesh>(res));
            }
            else if (res->GetResourceType() == RES_T_NORMAL)
            {
                m.m_normal_maps_.push_back(boost::static_pointer_cast<NormalMap>(res));
            }
            else if (res->GetResourceType() == RES_T_TEX)
            {
                m.m_textures_.push_back(boost::static_pointer_cast<Texture>(res));
            }
            else if (res->GetResourceType() == RES_T_BONE_ANIM)
            {
                //m.m_animations_.push_back(boost::static_pointer_cast<Animation>(res));
            }
            else
            {
                throw std::runtime_error("Invalid resource type");
            }
        }

        m.m_bounding_box_ = m.m_meshes_[0]->GetBoundingBox();

        for (const auto& mesh : m.m_meshes_)
        {
            BoundingBox::CreateMerged(m.m_bounding_box_, m.m_bounding_box_, mesh->GetBoundingBox());
        }

        m.UpdateVertices();

        m.SetName(name);
        const auto ptr = boost::make_shared<Model>(m);
        GetResourceManager().AddResource(ptr);

        return ptr;
    }

    void Model::UpdateVertices()
    {
        m_cached_vertices_.clear();

        for (const auto& mesh : m_meshes_)
        {
            m_cached_vertices_.insert(m_cached_vertices_.end(), mesh->GetVertices().begin(), mesh->GetVertices().end());
        }
    }

    void Model::Load_INTERNAL()
    {
        if (GetPath().empty())
        {
            return;
        }

        const auto scene = s_importer_.ReadFile(
                                                GetPath().string(),
                                                aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
                                                aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder |
                                                aiProcess_PopulateArmatureData);

        if (scene == nullptr)
        {
            throw std::runtime_error(s_importer_.GetErrorString());
        }

        const std::string scene_name = scene->mRootNode->mName.C_Str();
        SetName(scene_name + "_MODEL");

        if (scene->HasMeshes())
        {
            const auto  shape_count = scene->mNumMeshes;

            std::vector<Vector3> total_vertices;

            for (auto i = 0; i < shape_count; ++i)
            {
                Resources::Shape shape;
                IndexCollection  indices;

                const auto shape_ = scene->mMeshes[i];
                const auto mesh_name = shape_->mName.C_Str();

                const std::string mesh_lookup_name = GetName() + "_" + mesh_name + "_" + std::to_string(i);

                if (const auto mesh = GetResourceManager().GetResource<Mesh>(mesh_lookup_name).lock())
                {
                    m_meshes_.push_back(mesh);
                    continue;
                }

                const auto v_count = shape_->mNumVertices;
                const auto f_count  = shape_->mNumFaces;
                const auto b_count = shape_->mNumBones;

                if (f_count == 0)
                {
                    continue;
                }

                // extract vertices
                for (auto j = 0; j < v_count; ++j)
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

                    const auto vtx = VertexElement
                                    {
                                        {vec.x, vec.y, vec.z},
                                        {1.0f, 0.f, 0.f, 1.f},
                                        tex_coord,
                                        normal_,
                                        tangent_,
                                        binormal_
                                    };

                    shape.emplace_back(vtx);
                    total_vertices.push_back({vec.x, vec.y, vec.z});
                }

                for (auto j = 0; j < f_count; ++j)
                {
                    const auto face = shape_->mFaces[j];
                    const auto indices_ = face.mNumIndices;

                    // extract indices
                    for (int k = 0; k < indices_; ++k)
                    {
                        indices.push_back(face.mIndices[k]);
                    }
                }

                if (shape_->HasBones())
                {
                    BonePrimitiveMap bone_map;

                    for (auto j = 0; j < b_count; ++j)
                    {
                        const auto bone   = shape_->mBones[j];
                        const auto offset = bone->mOffsetMatrix;
                        const std::string bone_name = bone->mName.C_Str();

                        if (const auto check = GetResourceManager().GetResource<Bone>(mesh_lookup_name + "_BONE").lock())
                        {
                            m_bone_ = check;
                            break;
                        }

                        const auto weight_count = bone->mNumWeights;
                        // todo: expand
                        for (int influence = 0; influence < weight_count; ++influence)
                        {
                            const auto weight = bone->mWeights[influence];
                            const auto vertex_id = weight.mVertexId;
                            const auto weight_ = weight.mWeight;
                            auto& vtx_bone = shape[vertex_id].bone_element;

                            vtx_bone.Append(j, weight_);
                        }

                        const auto parent = bone->mNode->mParent;
                        const auto parent_name = parent->mName.C_Str();
                        int parent_idx = -1;

                        if (bone_map.contains(parent_name))
                        {
                            parent_idx = bone_map[parent_name].GetIndex();
                        }

                        BonePrimitive bone_info;
                        bone_info.SetIndex(j);
                        bone_info.SetParentIndex(parent_idx);
                        bone_info.SetInvBindPose(AiMatrixToDirectXTranspose(offset));
                        bone_info.SetTransform(AiMatrixToDirectXTranspose(bone->mNode->mTransformation));

                        bone_map[bone_name] = bone_info;
                    }

                    StrongBone bone = boost::make_shared<Bone>(bone_map);
                    bone->SetName(mesh_lookup_name + "_BONE");
                    GetResourceManager().AddResource(bone);
                    bone->Load();
                    m_bone_ = bone;
                }

                StrongMesh mesh = boost::make_shared<Mesh>(shape, indices);
                mesh->SetName(mesh_lookup_name);
                mesh->Load();
                GetResourceManager().AddResource(mesh);
                m_meshes_.push_back(mesh);
            }

            for (const auto& mesh : m_meshes_)
            {
                for (const auto& vertex : mesh->m_vertices_)
                {
                    for (const auto& idx : vertex.bone_element.GetBoneIndices())
                    {
                        const auto unique = std::ranges::find_if(
                                             m_bone_vertices_[idx],
                                        [vertex](const Vector3& v)
                        {
                            return FloatCompare(v.x, vertex.position.x) &&
                                   FloatCompare(v.y, vertex.position.y) &&
                                   FloatCompare(v.z, vertex.position.z);
                        });

                        if (unique != m_bone_vertices_[idx].end())
                        {
                            continue;
                        }
                        
                        m_bone_vertices_[idx].push_back(vertex.position);
                    }
                }
            }

            for (const auto& [idx, vertices] : m_bone_vertices_)
            {
                BoundingOrientedBox::CreateFromPoints(m_bone_bounding_boxes_[idx], vertices.size(), vertices.data(), sizeof(Vector3));
            }

            if (scene->HasAnimations())
            {
                const auto animation_count = scene->mNumAnimations;

                for (auto j = 0; j < animation_count; ++j)
                {
                    const auto animation_ = scene->mAnimations[j];
                    const auto affect_bone_count = animation_->mNumChannels;
                    const std::string anim_name = animation_->mName.C_Str();

                    if (const auto check = GetResourceManager().GetResource<BoneAnimation>(anim_name + "_ANIM").lock())
                    {
                        m_animations_.push_back(check);
                        continue;
                    }

                    const auto duration = animation_->mDuration;
                    const auto ticks_per_second = animation_->mTicksPerSecond == 0 ? 25.f : animation_->mTicksPerSecond;

                    AnimationPrimitive animation(
                                                 anim_name, duration, ticks_per_second,
                                                 AiMatrixToDirectXTranspose(
                                                                            scene->mRootNode->mTransformation.
                                                                            Inverse()));

                    for (auto k = 0; k < affect_bone_count; ++k)
                    {
                        const auto channel = animation_->mChannels[k];
                        const auto bone_name = channel->mNodeName;

                        const auto bone     = m_bone_->GetBone(bone_name.C_Str());

                        if (!bone)
                        {
                            // todo: recover? multiplying inv_bind_pose * global_transform * global_inverse_transform
                            // might be enough to get the final transform
                            continue;
                        }

                        BoneAnimationPrimitive bone_animation;
                        bone_animation.SetIndex(bone->GetIndex());

                        const auto positions = channel->mNumPositionKeys;
                        for (auto l = 0; l < positions; ++l)
                        {
                            const auto key = channel->mPositionKeys[l];
                            const auto time = static_cast<float>(key.mTime);
                            const auto value = key.mValue;

                            bone_animation.AddPosition(time, Vector3{value.x, value.y, value.z});
                        }

                        const auto rotations = channel->mNumRotationKeys;
                        for (auto l = 0; l < rotations; ++l)
                        {
                            const auto key = channel->mRotationKeys[l];
                            const auto time = static_cast<float>(key.mTime);
                            const auto value = key.mValue;

                            bone_animation.AddRotation(time, Quaternion{value.x, value.y, value.z, value.w});
                        }

                        const auto scalings = channel->mNumScalingKeys;
                        for (auto l = 0; l < scalings; ++l)
                        {
                            const auto key = channel->mScalingKeys[l];
                            const auto time = static_cast<float>(key.mTime);
                            const auto value = key.mValue;

                            bone_animation.AddScale(time, Vector3{value.x, value.y, value.z});
                        }

                        animation.Add(bone_name.C_Str(), bone_animation);
                    }

                    StrongBoneAnimation anim = boost::make_shared<BoneAnimation>(animation);
                    anim->SetName(anim_name + "_ANIM");
                    anim->BindBone(m_bone_);
                    anim->Load();
                    GetResourceManager().AddResource(anim);
                    m_animations_.push_back(anim);
                }
            }

            UpdateVertices();

            BoundingBox::CreateFromPoints(m_bounding_box_, total_vertices.size(), total_vertices.data(), sizeof(Vector3));
        }
        else
        {
            throw std::runtime_error("No meshes found in file");
        }
    }

    void Model::Unload_INTERNAL()
    {
    }

    Model::Model()
    : Resource("", RES_T_MODEL),
      m_bounding_box_({}) {}
}

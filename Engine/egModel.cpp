#include "pch.h"
#include "egModel.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include "egResourceManager.hpp"
#include "egMesh.h"
#include "egNormalMap.h"
#include "egAnimation.h"
#include "egBone.h"

namespace Engine::Resources
{
    Model::Model(const std::filesystem::path& path)
    : Resource(path, RES_T_MODEL),
      m_render_index_(0), m_bounding_box_({}) {}

    void Model::PreUpdate(const float& dt) {}

    void Model::Update(const float& dt) {}

    void Model::FixedUpdate(const float& dt) {}

    void Model::PreRender(const float& dt) {}

    void Model::Render(const float& dt)
    {
        if (!m_normal_maps_.empty()) m_normal_maps_[m_render_index_ % m_normal_maps_.size()]->Render(dt);
        if (!m_textures_.empty()) m_textures_[m_render_index_ % m_textures_.size()]->Render(dt);
        if (m_bone_) m_bone_->Render(dt);
        // todo: expand
        if (!m_animations_.empty()) 
        {
            m_animations_[0]->SetFrame(dt);
            m_animations_[0]->Render(dt);
        }

        m_meshes_[m_render_index_]->Render(dt);
        m_render_index_++;

        GetRenderPipeline().UnbindResource(SR_NORMAL_MAP);
        GetRenderPipeline().UnbindResource(SR_TEXTURE);
        GetRenderPipeline().UnbindResource(SR_ANIMATION);
    }

    void Model::PostRender(const float& dt) {}

    void Model::RenderMeshOnly(const float& dt)
    {
        m_meshes_[m_render_index_]->Render(dt);
        m_render_index_++;
    }

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

    UINT Model::GetRenderIndex() const
    {
        return m_render_index_;
    }

    UINT Model::GetRemainingRenderIndex() const
    {
        return m_meshes_.size() - m_render_index_ <= 0 ? 0 : m_meshes_.size() - m_render_index_;
    }

    void Model::ResetRenderIndex()
    {
        m_render_index_ = 0;
    }

    StrongModel Model::Create(const std::string& name, const std::vector<StrongResource>& resources) {
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
            else if (res->GetResourceType() == RES_T_ANIM)
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

            for (int i = 0; i < shape_count; ++i)
            {
                Resources::Shape shape;
                IndexCollection  indices;

                const auto shape_ = scene->mMeshes[i];
                const auto mesh_name = shape_->mName.C_Str();

                const std::string mesh_lookup_name = GetName() + "_" + mesh_name;

                if (const auto mesh = GetResourceManager().GetResource<Mesh>(mesh_lookup_name).lock())
                {
                    m_meshes_.push_back(mesh);
                    continue;
                }

                const auto v_count = shape_->mNumVertices;
                const auto f_count  = shape_->mNumFaces;
                const auto b_count = shape_->mNumBones;

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

                if (shape_->HasBones())
                {
                    BonePrimitiveMap bone_map;

                    // exceptional case: root bone (rig)
                    BonePrimitive root_bone;
                    root_bone.idx = 0;
                    root_bone.parent_idx = -1;
                    root_bone.offset = Matrix::Identity;

                    bone_map[shape_->mBones[0]->mNode->mParent->mName.C_Str()] = root_bone;

                    for (int j = 0; j < b_count; ++j)
                    {
                        BonePrimitive bone_info;

                        const auto bone   = shape_->mBones[j];
                        const auto offset = bone->mOffsetMatrix;
                        const std::string bone_name = bone->mName.C_Str();

                        if (const auto check = GetResourceManager().GetResource<Bone>(mesh_lookup_name + "_BONE").lock())
                        {
                            m_bone_ = check;
                            continue;
                        }

                        const auto parent = bone->mNode->mParent;

                        bone_info.idx     = j + 1;
                        bone_info.offset = AiMatrixToDirectX(offset);

                        if (j == 0)
                        {
                            bone_info.parent_idx = 0;
                        }
                        else                         
                        {
                            const auto parent_name = parent->mName.C_Str();
                            bone_info.parent_idx = bone_map[parent_name].idx;
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

            if (scene->HasAnimations())
            {
                const auto animation_count = scene->mNumAnimations;
                const aiMatrix4x4 ai_scene_inv = scene->mRootNode->mTransformation.Inverse();
                const Matrix scene_inv_transform = AiMatrixToDirectX(ai_scene_inv);

                for (int j = 0; j < animation_count; ++j)
                {
                    AnimationPrimitive animation;
                    animation.bone_animations.resize(m_bone_->GetBoneCount());

                    const auto animation_ = scene->mAnimations[j];
                    const auto affect_bone_count = animation_->mNumChannels;
                    const std::string anim_name = animation_->mName.C_Str();

                    if (const auto check = GetResourceManager().GetResource<Animation>(anim_name + "_ANIM").lock())
                    {
                        m_animations_.push_back(check);
                        continue;
                    }

                    const auto duration = animation_->mDuration;
                    const auto ticks_per_second = animation_->mTicksPerSecond == 0 ? 25.f : animation_->mTicksPerSecond;

                    animation.duration = duration;
                    animation.ticks_per_second = ticks_per_second;
                    animation.name = anim_name;
                    animation.global_inverse_transform = scene_inv_transform;

                    for (int k = 0; k < affect_bone_count; ++k)
                    {
                        BoneAnimation bone_animation;
                        
                        const auto channel = animation_->mChannels[k];
                        const auto bone_name = channel->mNodeName;
                        const auto bone_idx = m_bone_->GetBone(bone_name.C_Str()).idx;
                        const auto parent = m_bone_->GetBone(bone_name.C_Str()).parent_idx;

                        const auto positions = channel->mNumPositionKeys;
                        for (auto l = 0; l < positions; ++l)
                        {
                            const auto key = channel->mPositionKeys[l];
                            const auto time = key.mTime;
                            const auto value = key.mValue;

                            bone_animation.position.emplace_back(time, Vector3{value.x, value.y, value.z});
                        }

                        const auto rotations = channel->mNumRotationKeys;
                        for (auto l = 0; l < rotations; ++l)
                        {
                            const auto key = channel->mRotationKeys[l];
                            const auto time = key.mTime;
                            const auto value = key.mValue;

                            bone_animation.rotation.emplace_back(time, Quaternion{value.x, value.y, value.z, value.w});
                        }

                        const auto scalings = channel->mNumScalingKeys;
                        for (auto l = 0; l < scalings; ++l)
                        {
                            const auto key = channel->mScalingKeys[l];
                            const auto time = key.mTime;
                            const auto value = key.mValue;

                            bone_animation.scale.emplace_back(time, Vector3{value.x, value.y, value.z});
                        }

                        animation.bone_animations[bone_idx] = bone_animation;
                    }

                    StrongAnimation anim = boost::make_shared<Animation>(animation);
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
      m_render_index_(0), m_bounding_box_({}) {}
}

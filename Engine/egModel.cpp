#include "pch.h"
#include "egModel.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include "egResourceManager.hpp"
#include "egMesh.h"
#include "egAnimation.h"
#include "egNormalMap.h"

SERIALIZER_ACCESS_IMPL(Engine::Resources::Model,
                       _ARTAG(_BSTSUPER(Resource))
                       _ARTAG(m_meshes_)
                       _ARTAG(m_animations_)
                       _ARTAG(m_bone_map_)
                       _ARTAG(m_textures_)
                       _ARTAG(m_normal_maps_)
                       _ARTAG(m_bounding_box_))

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
        //m_animations_[m_render_index_ % m_animations_.size()]->Render(dt);
        m_meshes_[m_render_index_]->Render(dt);
        m_render_index_++;

        GetRenderPipeline().UnbindResource(SR_NORMAL_MAP);
        GetRenderPipeline().UnbindResource(SR_TEXTURE);
        GetRenderPipeline().UnbindResource(SR_BONE);
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
                m.m_animations_.push_back(boost::static_pointer_cast<Animation>(res));
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
                                                aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder | aiProcess_PopulateArmatureData);

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

            for (int i = shape_count - 1; i >= 0; --i)
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
                    for (int j = 0; j < b_count; ++j)
                    {
                        BonePrimitive bone_info;

                        const auto bone   = shape_->mBones[j];
                        const auto offset = bone->mOffsetMatrix;
                        const std::string bone_name = bone->mName.C_Str();

                        bone_info.name    = bone_name;
                        bone_info.idx     = j;
                        bone_info.offset  = Matrix
                        {
                            offset.a1, offset.a2, offset.a3, offset.a4,
                            offset.b1, offset.b2, offset.b3, offset.b4,
                            offset.c1, offset.c2, offset.c3, offset.c4,
                            offset.d1, offset.d2, offset.d3, offset.d4
                        };

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

                        m_bone_map_[bone_name] = bone_info;
                    }
                }

                if (scene->HasAnimations())
                {
                    const auto animation_count = scene->mNumAnimations;

                    for (int j = 0; j < animation_count; ++j)
                    {
                        AnimationPrimitive animation;

                        const auto animation_ = scene->mAnimations[j];
                        const auto channel_count = animation_->mNumChannels;
                        animation.name = animation_->mName.C_Str();
                        const auto anim_lookup_name = GetName() + "_" + animation.name;

                        if (const auto anim = GetResourceManager().GetResource<Animation>(anim_lookup_name).lock())
                        {
                            m_animations_.push_back(anim);
                            continue;
                        }

                        for (int k = 0; k < channel_count; ++k)
                        {
                            const auto channel = animation_->mChannels[k];
                            const auto transform_count = channel->mNumPositionKeys;

                            for (int l = 0; l < transform_count; ++l)
                            {
                                const auto frame = static_cast<float>(channel->mPositionKeys[l].mTime);
                                const auto t = channel->mPositionKeys[l].mValue;
                                const auto r = channel->mRotationKeys[l].mValue;
                                const auto s = channel->mScalingKeys[l].mValue;

                                animation.keyframes.push_back(KeyFrame{
                                    frame,
                                    Vector3(s.x, s.y, s.z),
                                    Quaternion(r.x, r.y, r.z, r.w),
                                    Vector3(t.x, t.y, t.z)});
                            }
                        }

                        StrongAnimation anim = boost::make_shared<Animation>(animation);
                        anim->SetName(anim_lookup_name);
                        GetResourceManager().AddResource(anim);
                        m_animations_.push_back(anim);
                    }
                }

                StrongMesh mesh = boost::make_shared<Mesh>(shape, indices);
                mesh->SetName(mesh_lookup_name);
                mesh->Load();

                GetResourceManager().AddResource(mesh);
                m_meshes_.push_back(mesh);
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

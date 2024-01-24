#include "pch.h"
#include "egShape.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "egAnimationsTexture.h"
#include "egBaseAnimation.h"
#include "egBone.h"
#include "egBoneAnimation.h"
#include "egMesh.h"
#include "egResourceManager.hpp"

SERIALIZER_ACCESS_IMPL
(
 Engine::Resources::Shape,
 _ARTAG(_BSTSUPER(Resource))
 _ARTAG(m_bone_)
 _ARTAG(m_meshes_)
 _ARTAG(m_animation_catalog_)
 _ARTAG(m_bounding_box_)
 _ARTAG(m_bone_bounding_boxes_)
)

namespace Engine::Resources
{
  Shape::Shape(const std::filesystem::path& path)
    : Resource(path, RES_T_SHAPE),
      m_bounding_box_({}) {}

  void Shape::PreUpdate(const float& dt) {}

  void Shape::Update(const float& dt) {}

  void Shape::FixedUpdate(const float& dt) {}

  void Shape::PreRender(const float& dt) { for (const auto& mesh : m_meshes_) { mesh->PreRender(dt); } }

  void Shape::Render(const float& dt)
  {
    if (m_animations_)
    {
      m_animations_->Render(dt);
    }
    
    for (const auto& mesh : m_meshes_) { mesh->Render(dt); }
  }

  void Shape::PostRender(const float& dt)
  {
    if (m_animations_)
    {
      m_animations_->PostRender(dt);
    }

    for (const auto& mesh : m_meshes_) { mesh->PostRender(dt); }
  }

  void Shape::PostUpdate(const float& dt) {}

  void Shape::OnDeserialized()
  {
    Resource::OnDeserialized();
    Load();
  }

  BoundingBox Shape::GetBoundingBox() const { return m_bounding_box_; }

  WeakMesh Shape::GetMesh(const std::string& name) const
  {
    const auto it = std::ranges::find_if
      (
       m_meshes_
       , [&name](const auto& mesh) { return mesh->GetName() == name; }
      );

    if (it != m_meshes_.end()) { return *it; }

    return {};
  }

  WeakMesh Shape::GetMesh(const UINT index) const
  {
    if (m_meshes_.size() > index) { return m_meshes_[index]; }

    return {};
  }

  WeakAnimsTexture Shape::GetAnimations() const
  {
    return m_animations_;
  }

  const std::vector<VertexElement>& Shape::GetVertices() const { return m_cached_vertices_; }

  std::vector<StrongMesh> Shape::GetMeshes() const { return m_meshes_; }

  const std::vector<std::string>& Shape::GetAnimationCatalog() const { return m_animation_catalog_; }

  const std::map<UINT, BoundingOrientedBox>& Shape::GetBoneBoundingBoxes() const { return m_bone_bounding_boxes_; }

  void Shape::UpdateVertices()
  {
    m_cached_vertices_.clear();

    for (const auto& mesh : m_meshes_)
    {
      for (const auto& vertex : mesh->GetVertexCollection()) { m_cached_vertices_.push_back(vertex); }
    }
  }

  void Shape::Load_INTERNAL()
  {
    if (GetPath().empty()) { return; }

    const auto scene = s_importer_.ReadFile
      (
       GetPath().string(),
       aiProcess_Triangulate | aiProcess_GenSmoothNormals |
       aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
       aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder |
       aiProcess_PopulateArmatureData
      );

    if (scene == nullptr) { throw std::runtime_error(s_importer_.GetErrorString()); }

    aiMatrix4x4 axis = {
      1.f, 0.f, 0.f, 0.f,
      0.f, 0.f, -1.f, 0.f,
      0.f, -1.f, 0.f, 0.f,
      0.f, 0.f, 0.f, 1.f
    };

    scene->mRootNode->mTransformation = axis;

    if (scene->HasMeshes())
    {
      const auto shape_count = scene->mNumMeshes;

      std::vector<Vector3> total_vertices;

      for (auto i = 0; i < shape_count; ++i)
      {
        VertexCollection shape;
        IndexCollection  indices;

        const auto shape_    = scene->mMeshes[i];
        const auto mesh_name = shape_->mName.C_Str();

        const std::string mesh_lookup_name = GetName() + "_" + mesh_name + "_" + std::to_string(i);

        if (const auto mesh = GetResourceManager().GetResource<Mesh>(mesh_lookup_name).lock())
        {
          m_meshes_.push_back(mesh);
          continue;
        }

        const auto v_count = shape_->mNumVertices;
        const auto f_count = shape_->mNumFaces;
        const auto b_count = shape_->mNumBones;

        if (f_count == 0) { continue; }

        // extract vertices
        for (auto j = 0; j < v_count; ++j)
        {
          auto vec = shape_->mVertices[j];
          vec *= axis;

          Vector2 tex_coord = {0.f, 0.f};
          Vector3 normal_   = {0.f, 0.f, 0.f};
          Vector3 tangent_  = {0.f, 0.f, 0.f};
          Vector3 binormal_ = {0.f, 0.f, 0.f};

          if (shape_->HasTextureCoords(j))
          {
            const auto tex = shape_->mTextureCoords[j]; // Assuming UV exists in 2D
            tex_coord      = Vector2{tex->x, tex->y};
          }

          if (shape_->HasNormals())
          {
            auto normal = shape_->mNormals[j];
            normal *= axis;
            normal_ = Vector3{normal.x, normal.y, normal.z};
          }

          if (shape_->HasTangentsAndBitangents())
          {
            auto tangent  = shape_->mTangents[j];
            auto binormal = shape_->mBitangents[j];

            tangent *= axis;
            binormal *= axis;

            tangent_  = Vector3{tangent.x, tangent.y, tangent.z};
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
          const auto face     = shape_->mFaces[j];
          const auto indices_ = face.mNumIndices;

          // extract indices
          for (int k = 0; k < indices_; ++k) { indices.push_back(face.mIndices[k]); }
        }

        if (shape_->HasBones())
        {
          BonePrimitiveMap bone_map;

          for (auto j = 0; j < b_count; ++j)
          {
            const auto bone           = shape_->mBones[j];
            auto       offset         = bone->mOffsetMatrix;
            auto       transformation = bone->mNode->mTransformation;

            offset *= axis;
            transformation *= axis;

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
              const auto weight    = bone->mWeights[influence];
              const auto vertex_id = weight.mVertexId;
              const auto weight_   = weight.mWeight;
              auto&      vtx_bone  = shape[vertex_id].bone_element;

              vtx_bone.Append(j, weight_);
            }

            const auto parent      = bone->mNode->mParent;
            const auto parent_name = parent->mName.C_Str();
            int        parent_idx  = -1;

            if (bone_map.contains(parent_name)) { parent_idx = bone_map[parent_name].GetIndex(); }

            BonePrimitive bone_info;
            bone_info.SetIndex(j);
            bone_info.SetParentIndex(parent_idx);
            bone_info.SetInvBindPose(AiMatrixToDirectXTranspose(offset));
            bone_info.SetTransform(AiMatrixToDirectXTranspose(transformation));

            bone_map[bone_name] = bone_info;
          }

          auto bone = boost::make_shared<Bone>(bone_map);
          bone->SetName(mesh_lookup_name + "_BONE");
          GetResourceManager().AddResource(bone);
          bone->Load();
          m_bone_ = bone;
        }

        auto mesh = boost::make_shared<Mesh>(shape, indices);
        mesh->SetName(mesh_lookup_name);
        mesh->Load();
        GetResourceManager().AddResource(mesh);
        m_meshes_.push_back(mesh);
      }

      std::map<UINT, std::vector<Vector3>> bone_vertices;

      for (const auto& mesh : m_meshes_)
      {
        for (const auto& vertex : mesh->GetVertexCollection())
        {
          for (const auto& idx : vertex.bone_element.GetIndices())
          {
            const auto unique = std::ranges::find_if
              (
               bone_vertices[idx],
               [vertex](const Vector3& v)
               {
                 return FloatCompare(v.x, vertex.position.x) &&
                        FloatCompare(v.y, vertex.position.y) &&
                        FloatCompare(v.z, vertex.position.z);
               }
              );

            if (unique != bone_vertices[idx].end()) { continue; }

            bone_vertices[idx].push_back(vertex.position);
          }
        }
      }

      for (const auto& [idx, vertices] : bone_vertices)
      {
        BoundingOrientedBox::CreateFromPoints
          (m_bone_bounding_boxes_[idx], vertices.size(), vertices.data(), sizeof(Vector3));
      }

      std::vector<StrongBoneAnimation> animations;

      if (scene->HasAnimations())
      {
        const auto animation_count = scene->mNumAnimations;

        for (auto j = 0; j < animation_count; ++j)
        {
          const auto        animation_        = scene->mAnimations[j];
          const auto        affect_bone_count = animation_->mNumChannels;
          const std::string anim_name         = animation_->mName.C_Str();

          if (const auto check = GetResourceManager().GetResource<BoneAnimation>(anim_name + "_ANIM").lock())
          {
            m_animation_catalog_.push_back(anim_name + "_ANIM");
            continue;
          }

          const auto duration         = animation_->mDuration;
          const auto ticks_per_second = animation_->mTicksPerSecond == 0 ? 25.f : animation_->mTicksPerSecond;

          AnimationPrimitive animation
            (
             anim_name, duration, ticks_per_second,
             AiMatrixToDirectXTranspose
             (
              scene->mRootNode->mTransformation.
                     Inverse()
             )
            );

          for (auto k = 0; k < affect_bone_count; ++k)
          {
            const auto channel   = animation_->mChannels[k];
            const auto bone_name = channel->mNodeName;

            const auto bone = m_bone_->GetBone(bone_name.C_Str());

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
              const auto key   = channel->mPositionKeys[l];
              const auto time  = static_cast<float>(key.mTime);
              const auto value = key.mValue;

              bone_animation.AddPosition(time, Vector3{value.x, value.y, value.z});
            }

            const auto rotations = channel->mNumRotationKeys;
            for (auto l = 0; l < rotations; ++l)
            {
              const auto key   = channel->mRotationKeys[l];
              const auto time  = static_cast<float>(key.mTime);
              const auto value = key.mValue;

              bone_animation.AddRotation(time, Quaternion{value.x, value.y, value.z, value.w});
            }

            const auto scalings = channel->mNumScalingKeys;
            for (auto l = 0; l < scalings; ++l)
            {
              const auto key   = channel->mScalingKeys[l];
              const auto time  = static_cast<float>(key.mTime);
              const auto value = key.mValue;

              bone_animation.AddScale(time, Vector3{value.x, value.y, value.z});
            }

            animation.Add(bone_name.C_Str(), bone_animation);
          }

          const auto anim = boost::make_shared<BoneAnimation>(animation);
          anim->SetName(anim_name + "_ANIM");
          anim->BindBone(m_bone_);
          anim->Load();
          GetResourceManager().AddResource(anim);
          m_animation_catalog_.push_back(anim_name + "_ANIM");
          animations.push_back(anim);
        }

        const auto anims = boost::make_shared<AnimationsTexture>(animations);
        anims->SetName(GetName() + "_ANIMS");
        anims->Load();
        GetResourceManager().AddResource(anims);
        m_animations_ = anims;
      }

      UpdateVertices();

      BoundingBox::CreateFromPoints(m_bounding_box_, total_vertices.size(), total_vertices.data(), sizeof(Vector3));
    }
    else { throw std::runtime_error("No meshes found in file"); }
  }

  void Shape::Unload_INTERNAL()
  {
    m_meshes_.clear();
    m_animation_catalog_.clear();
    m_bone_bounding_boxes_.clear();
    m_cached_vertices_.clear();
    m_bounding_box_ = {};
    m_bone_.reset();
  }

  Shape::Shape()
    : Resource("", RES_T_SHAPE),
      m_bounding_box_({}) {}
}

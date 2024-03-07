#include "pch.h"
#include "egBoneAnimation.h"

#include "egBone.h"

namespace Engine::Graphics
{
  struct BoneTransformElement;
}

SERIALIZER_ACCESS_IMPL
(
 Engine::Resources::BoneAnimation,
 _ARTAG(_BSTSUPER(BaseAnimation))
 _ARTAG(m_primitive_)
 _ARTAG(m_bone_)
 _ARTAG(m_bone_meta_path_str_)
)

namespace Engine::Resources
{
  BoneAnimation::BoneAnimation(const AnimationPrimitive& primitive)
    : BaseAnimation(),
      m_primitive_(primitive),
      m_evaluated_time_(0) {}

  void BoneAnimation::PreUpdate(const float& dt) {}

  void BoneAnimation::Update(const float& dt) {}

  void BoneAnimation::FixedUpdate(const float& dt) {}

  void BoneAnimation::PreRender(const float& dt) {}

  void BoneAnimation::Render(const float& dt) {}

  void BoneAnimation::PostRender(const float& dt) {}

  void BoneAnimation::PostUpdate(const float& dt) {}

  void BoneAnimation::OnSerialized()
  {
    BaseAnimation::OnSerialized();
    Serializer::Serialize(m_bone_->GetName(), m_bone_);
    m_bone_meta_path_str_ = m_bone_->GetMetadataPath().string();
  }

  void BoneAnimation::OnDeserialized()
  {
    BaseAnimation::OnDeserialized();

    if (const auto res_check = Resources::Bone::GetByMetadataPath(m_bone_meta_path_str_).lock(); 
        res_check && !res_check->GetMetadataPath().empty())
    {
      m_bone_ = res_check;
    }
    else
    {
      m_bone_ = GetResourceManager().GetResourceByMetadataPath<Resources::Bone>(m_bone_meta_path_str_).lock();
    }

    m_primitive_.RebuildIndexCache();
  }

  void BoneAnimation::BindBone(const WeakBone& bone_info)
  {
    if (const auto locked = bone_info.lock()) { m_bone_ = locked; }
  }

  eResourceType BoneAnimation::GetResourceType() const { return RES_T_BONE_ANIM; }

  std::vector<Matrix> BoneAnimation::GetFrameAnimationDt(const float dt)
  {
    const auto anim_time = ConvertDtToFrame(dt, m_primitive_.GetTicksPerSecond(), m_primitive_.GetDuration());
    return GetFrameAnimation(anim_time);
  }

  std::vector<Matrix> BoneAnimation::GetFrameAnimation(const float time)
  {
    if (time != 0.f && m_evaluated_time_ == time && !m_evaluated_data_.empty()) { return m_evaluated_data_; }

    m_evaluated_data_.clear();
    m_evaluated_time_ = time;
    
    std::vector<Matrix> memo;

    memo.clear();
    memo.resize(m_primitive_.GetBoneCount());

    for (int i = 0; i < m_primitive_.GetBoneCount(); ++i)
    {
      Matrix                  bfa;
      const BoneAnimationPrimitive* bone_animation = m_primitive_.GetBoneAnimation(i);
      const BonePrimitive*          bone           = m_bone_->GetBone(i);
      const BonePrimitive*          parent         = m_bone_->GetBoneParent(i);

      const auto position = bone_animation->GetPosition(time);
      const auto rotation = bone_animation->GetRotation(time);
      const auto scale    = bone_animation->GetScale(time);

      const Matrix vertex_transform = Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion
                                      (rotation) * Matrix::CreateTranslation(position);

      Matrix parent_transform = Matrix::Identity;

      if (parent) { parent_transform = memo[parent->GetIndex()]; }

      const Matrix node_transform = vertex_transform;

      const Matrix global_transform = node_transform * parent_transform;
      memo[bone->GetIndex()]        = global_transform;

      const auto final_transform = bone->GetInvBindPose() * global_transform * m_primitive_.GetGlobalInverseTransform();
      bfa              = final_transform;
      m_evaluated_data_.push_back(bfa);
    }

    return m_evaluated_data_;
  }

  void BoneAnimation::Load_INTERNAL()
  {
    SetDuration(m_primitive_.GetDuration());
    SetTicksPerSecond(m_primitive_.GetTicksPerSecond());
  }

  void BoneAnimation::Unload_INTERNAL() { }

  BoneAnimation::BoneAnimation()
    : BaseAnimation(),
      m_primitive_(),
      m_evaluated_time_(0) { }
}

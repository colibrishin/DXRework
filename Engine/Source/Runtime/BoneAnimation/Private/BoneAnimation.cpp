#include "BoneAnimation.h"

#include "Bone.h"
#include "BoneAnimation.generated.h"

namespace Engine::Resources
{
	BoneAnimation::BoneAnimation(const AnimationPrimitive& primitive)
		: BaseAnimation(),
		  m_primitive_(primitive),
		  m_evaluated_time_(0) {}

	void BoneAnimation::PreUpdate(const float dt) {}

	void BoneAnimation::Update(const float dt) {}

	void BoneAnimation::FixedUpdate(const float dt) {}

	void BoneAnimation::PostUpdate(const float dt) {}

	void BoneAnimation::OnSerialized()
	{
		BaseAnimation::OnSerialized();

		if (const Strong<Bone>& bone = m_cached_bone_.lock())
		{
			Serializer::Serialize(bone->GetName(), bone);
			m_bone_path_ = bone->GetMetadataPath();
		}
	}

	void BoneAnimation::OnDeserialized()
	{
		BaseAnimation::OnDeserialized();

		if (const Strong<Bone>& res_check = Bone::GetByMetadataPath(m_bone_path_).lock())
		{
			BindBone(res_check);
		}

		m_primitive_.RebuildIndexCache();
	}

	void BoneAnimation::BindBone(const Weak<Bone>& bone_info)
	{
		if (const auto locked = bone_info.lock())
		{
			m_bone_ = locked;
			m_cached_bone_ = locked;
			m_bone_path_ = m_bone_->GetMetadataPath();
		}
	}

	std::vector<Matrix> BoneAnimation::GetFrameAnimationDt(const float dt)
	{
		const auto anim_time = ConvertDtToFrame(dt, m_primitive_.GetTicksPerSecond());
		return GetFrameAnimation(anim_time);
	}

	std::vector<Matrix> BoneAnimation::GetFrameAnimation(const float time)
	{
		if (time != 0.f && m_evaluated_time_ == time && !m_evaluated_data_.empty())
		{
			return m_evaluated_data_;
		}

		m_evaluated_data_.clear();
		m_evaluated_time_ = time;

		std::vector<Matrix> memo;

		memo.clear();
		memo.resize(m_primitive_.GetBoneCount());

		if (const Strong<Bone>& locked_bone = m_cached_bone_.lock()) 
		{
			for (int i = 0; i < m_primitive_.GetBoneCount(); ++i)
			{
				Matrix                        bfa;
				const BoneAnimationPrimitive* bone_animation = m_primitive_.GetBoneAnimation(i);
				const BonePrimitive* bone = locked_bone->GetBone(i);
				const BonePrimitive* parent = locked_bone->GetBoneParent(i);

				const auto position = bone_animation->GetPosition(time);
				const auto rotation = bone_animation->GetRotation(time);
				const auto scale = bone_animation->GetScale(time);

				const Matrix vertex_transform = Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion
				(rotation) *Matrix::CreateTranslation(position);

				Matrix parent_transform = Matrix::Identity;

				if (parent)
				{
					parent_transform = memo[parent->GetIndex()];
				}

				const Matrix node_transform = vertex_transform;

				const Matrix global_transform = node_transform * parent_transform;
				memo[bone->GetIndex()] = global_transform;

				const auto final_transform = bone->GetInvBindPose() * global_transform * m_primitive_.
					GetGlobalInverseTransform();
				bfa = final_transform;
				m_evaluated_data_.push_back(bfa);
			}
		}

		return m_evaluated_data_;
	}

	void BoneAnimation::Load_INTERNAL()
	{
		SetDuration(m_primitive_.GetDuration());
		SetTicksPerSecond(m_primitive_.GetTicksPerSecond());

		if (const Strong<Bone>& bone = m_cached_bone_.lock()) 
		{
			m_bone_ = bone;
		}
	}

	void BoneAnimation::Unload_INTERNAL() 
	{
		m_bone_.reset();
	}

	BoneAnimation::BoneAnimation()
		: BaseAnimation(),
		  m_primitive_(),
		  m_evaluated_time_(0) { }
}

#include "../Public/BoneAnimation.h"

#include "Source/Runtime/Resources/Bone/Public/Bone.h"
#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Graphics
{
	struct BoneTransformElement;
	AnimationPrimitive::AnimationPrimitive(): duration(0.f),
	                                          ticks_per_second(0.f)
	{
		RebuildIndexCache();
	}

	AnimationPrimitive::AnimationPrimitive(
		std::string name, float duration, float ticks_per_second, Matrix global_inverse_transform
	): name_(std::move(name)),
	   duration(duration),
	   ticks_per_second(ticks_per_second),
	   global_inverse_transform_(std::move(global_inverse_transform))
	{
		RebuildIndexCache();
	}

	AnimationPrimitive::AnimationPrimitive(const AnimationPrimitive& other) noexcept
	{
		name_                     = other.name_;
		duration                  = other.duration;
		ticks_per_second          = other.ticks_per_second;
		global_inverse_transform_ = other.global_inverse_transform_;
		bone_animations           = other.bone_animations;

		RebuildIndexCache();
	}

	AnimationPrimitive::AnimationPrimitive(AnimationPrimitive&& other) noexcept
	{
		name_                      = other.name_;
		duration                   = other.duration;
		ticks_per_second           = other.ticks_per_second;
		global_inverse_transform_  = other.global_inverse_transform_;
		bone_animations            = std::move(other.bone_animations);
		bone_animations_index_wise = std::move(other.bone_animations_index_wise);
	}

	AnimationPrimitive& AnimationPrimitive::operator=(const AnimationPrimitive& other) noexcept
	{
		name_                     = other.name_;
		duration                  = other.duration;
		ticks_per_second          = other.ticks_per_second;
		global_inverse_transform_ = other.global_inverse_transform_;
		bone_animations           = other.bone_animations;

		RebuildIndexCache();

		return *this;
	}

	void AnimationPrimitive::Add(const std::string& name, const BoneAnimationPrimitive& bone_animation)
	{
		bone_animations[name]                                 = bone_animation;
		bone_animations_index_wise[bone_animation.GetIndex()] = &bone_animations[name];
	}

	void AnimationPrimitive::SetGlobalInverseTransform(const Matrix& global_inverse_transform)
	{
		this->global_inverse_transform_ = global_inverse_transform;
	}

	size_t AnimationPrimitive::GetBoneCount() const noexcept
	{
		return bone_animations.size();
	}

	float AnimationPrimitive::GetDuration() const noexcept
	{
		return duration;
	}

	float                         AnimationPrimitive::GetTicksPerSecond() const noexcept
	{
		return ticks_per_second;
	}

	const Matrix&                 AnimationPrimitive::GetGlobalInverseTransform() const noexcept
	{
		return global_inverse_transform_;
	}

	const BoneAnimationPrimitive* AnimationPrimitive::GetBoneAnimation(const int idx) const
	{
		if (bone_animations_index_wise.contains(idx))
		{
			return bone_animations_index_wise.at(idx);
		}

		return nullptr;
	}

	const BoneAnimationPrimitive* AnimationPrimitive::GetBoneAnimation(const std::string& name) const
	{
		if (bone_animations.contains(name))
		{
			return &bone_animations.at(name);
		}

		return nullptr;
	}

	void                          AnimationPrimitive::RebuildIndexCache()
	{
		for (const auto& [key, value] : bone_animations)
		{
			bone_animations_index_wise[value.GetIndex()] = &bone_animations[key];
		}
	}
}

namespace Engine::Resources
{
	RESOURCE_SELF_INFER_GETTER_IMPL(BoneAnimation)

	BoneAnimation::BoneAnimation(const AnimationPrimitive& primitive)
		: BaseAnimation(),
		  m_primitive_(primitive),
		  m_evaluated_time_(0) {}

	void BoneAnimation::PreUpdate(const float& dt) {}

	void BoneAnimation::Update(const float& dt) {}

	void BoneAnimation::FixedUpdate(const float& dt) {}

	void BoneAnimation::PostUpdate(const float& dt) {}

	void BoneAnimation::OnSerialized()
	{
		BaseAnimation::OnSerialized();
		m_bone_meta_path_str_ = m_bone_->GetMetadataPath().string();
	}

	void BoneAnimation::OnDeserialized()
	{
		BaseAnimation::OnDeserialized();

		if (const auto res_check = Bone::GetByMetadataPath(m_bone_meta_path_str_).lock();
			res_check && !res_check->GetMetadataPath().empty())
		{
			m_bone_ = res_check;
		}

		m_primitive_.RebuildIndexCache();
	}

	void BoneAnimation::BindBone(const Weak<Bone>& bone_info)
	{
		if (const auto locked = bone_info.lock())
		{
			m_bone_ = locked;
		}
	}

	eResourceType BoneAnimation::GetResourceType() const
	{
		return RES_T_BONE_ANIM;
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

		for (int i = 0; i < m_primitive_.GetBoneCount(); ++i)
		{
			Matrix                        bfa;
			const BoneAnimationPrimitive* bone_animation = m_primitive_.GetBoneAnimation(i);
			const BonePrimitive*          bone           = m_bone_->GetBone(i);
			const BonePrimitive*          parent         = m_bone_->GetBoneParent(i);

			const auto position = bone_animation->GetPosition(time);
			const auto rotation = bone_animation->GetRotation(time);
			const auto scale    = bone_animation->GetScale(time);

			const Matrix vertex_transform = Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion
			                                (rotation) * Matrix::CreateTranslation(position);

			Matrix parent_transform = Matrix::Identity;

			if (parent)
			{
				parent_transform = memo[parent->GetIndex()];
			}

			const Matrix node_transform = vertex_transform;

			const Matrix global_transform = node_transform * parent_transform;
			memo[bone->GetIndex()]        = global_transform;

			const auto final_transform = bone->GetInvBindPose() * global_transform * m_primitive_.
			                             GetGlobalInverseTransform();
			bfa = final_transform;
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

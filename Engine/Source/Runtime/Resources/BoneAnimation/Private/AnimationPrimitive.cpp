#include "AnimationPrimitive.h"
#include "AnimationPrimitive.generated.h"

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


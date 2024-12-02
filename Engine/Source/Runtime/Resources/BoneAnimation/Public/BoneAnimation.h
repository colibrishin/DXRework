#pragma once
#include "Source/Runtime/Resources/BaseAnimation/Public/BaseAnimation.h"

namespace Engine::Graphics 
{
	struct AnimationPrimitive
	{
	public:
		AnimationPrimitive()
			: duration(0.f),
			ticks_per_second(0.f)
		{
			RebuildIndexCache();
		}

		AnimationPrimitive(std::string name, float duration, float ticks_per_second, Matrix global_inverse_transform)
			: name_(std::move(name)),
			duration(duration),
			ticks_per_second(ticks_per_second),
			global_inverse_transform_(std::move(global_inverse_transform))
		{
			RebuildIndexCache();
		}

		AnimationPrimitive(const AnimationPrimitive& other) noexcept
		{
			name_ = other.name_;
			duration = other.duration;
			ticks_per_second = other.ticks_per_second;
			global_inverse_transform_ = other.global_inverse_transform_;
			bone_animations = other.bone_animations;

			RebuildIndexCache();
		}

		AnimationPrimitive(AnimationPrimitive&& other) noexcept
		{
			name_ = other.name_;
			duration = other.duration;
			ticks_per_second = other.ticks_per_second;
			global_inverse_transform_ = other.global_inverse_transform_;
			bone_animations = std::move(other.bone_animations);
			bone_animations_index_wise = std::move(other.bone_animations_index_wise);
		}

		AnimationPrimitive& operator=(const AnimationPrimitive& other) noexcept
		{
			name_ = other.name_;
			duration = other.duration;
			ticks_per_second = other.ticks_per_second;
			global_inverse_transform_ = other.global_inverse_transform_;
			bone_animations = other.bone_animations;

			RebuildIndexCache();

			return *this;
		}

		void Add(const std::string& name, const BoneAnimationPrimitive& bone_animation)
		{
			bone_animations[name] = bone_animation;
			bone_animations_index_wise[bone_animation.GetIndex()] = &bone_animations[name];
		}

		void SetGlobalInverseTransform(const Matrix& global_inverse_transform)
		{
			this->global_inverse_transform_ = global_inverse_transform;
		}

		size_t GetBoneCount() const noexcept
		{
			return bone_animations.size();
		}

		float GetDuration() const noexcept
		{
			return duration;
		}

		float GetTicksPerSecond() const noexcept
		{
			return ticks_per_second;
		}

		const Matrix& GetGlobalInverseTransform() const noexcept
		{
			return global_inverse_transform_;
		}

		const BoneAnimationPrimitive* GetBoneAnimation(const int idx) const
		{
			if (bone_animations_index_wise.contains(idx))
			{
				return bone_animations_index_wise.at(idx);
			}

			return nullptr;
		}

		const BoneAnimationPrimitive* GetBoneAnimation(const std::string& name) const
		{
			if (bone_animations.contains(name))
			{
				return &bone_animations.at(name);
			}

			return nullptr;
		}

		void RebuildIndexCache()
		{
			for (const auto& [key, value] : bone_animations)
			{
				bone_animations_index_wise[value.GetIndex()] = &bone_animations[key];
			}
		}

	private:
		friend class boost::serialization::access;

		template <class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar& name_;
			ar& duration;
			ar& ticks_per_second;
			ar& global_inverse_transform_;
			ar& bone_animations;
		}

		std::string                                   name_;
		float                                         duration;
		float                                         ticks_per_second;
		Matrix                                        global_inverse_transform_;
		std::map<std::string, BoneAnimationPrimitive> bone_animations;
		std::map<int, BoneAnimationPrimitive*>        bone_animations_index_wise;
	};
}

namespace Engine::Resources
{
	using namespace Graphics;

	class BoneAnimation : public BaseAnimation
	{
	public:
		RESOURCE_T(RES_T_BONE_ANIM)

		BoneAnimation(const AnimationPrimitive& primitive);

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		void          BindBone(const Weak<Bone>& bone_info);
		eResourceType GetResourceType() const override;

		std::vector<Matrix> GetFrameAnimationDt(float dt);
		std::vector<Matrix> GetFrameAnimation(float time);

		RESOURCE_SELF_INFER_GETTER_DECL(BoneAnimation)

	protected:
		SERIALIZE_DECL

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		BoneAnimation();

		AnimationPrimitive m_primitive_;
		Strong<Bone>       m_bone_;
		MetadataPath       m_bone_meta_path_str_;

		// non-serialized
		float               m_evaluated_time_;
		std::vector<Matrix> m_evaluated_data_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::BoneAnimation)
BOOST_CLASS_EXPORT_KEY(Engine::Graphics::AnimationPrimitive)
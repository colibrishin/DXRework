#pragma once
#include <map>
#include "Source/Runtime/Resources/BaseAnimation/Public/BaseAnimation.h"

namespace Engine::Graphics 
{
	struct ENGINE_BONEANIMATION_API AnimationPrimitive
	{
	public:
		AnimationPrimitive();
		AnimationPrimitive(std::string name, float duration, float ticks_per_second, Matrix global_inverse_transform);
		AnimationPrimitive(const AnimationPrimitive& other) noexcept;
		AnimationPrimitive(AnimationPrimitive&& other) noexcept;
		AnimationPrimitive& operator=(const AnimationPrimitive& other) noexcept;

		void Add(const std::string& name, const BoneAnimationPrimitive& bone_animation);
		void SetGlobalInverseTransform(const Matrix& global_inverse_transform);
		size_t GetBoneCount() const noexcept;
		float GetDuration() const noexcept;

		float GetTicksPerSecond() const noexcept;
		const Matrix& GetGlobalInverseTransform() const noexcept;
		const BoneAnimationPrimitive* GetBoneAnimation(const int idx) const;
		const BoneAnimationPrimitive* GetBoneAnimation(const std::string& name) const;
		void RebuildIndexCache();

	private:
		std::string                                   name_;
		float                                         duration;
		float                                         ticks_per_second;
		Matrix                                        global_inverse_transform_;
		std::map<std::string, BoneAnimationPrimitive> bone_animations;
		std::map<int, BoneAnimationPrimitive*>        bone_animations_index_wise;
	};
	
}

POLYMORPHIC_TYPE_MAP(ENGINE_BONEANIMATION_API, Engine::Resources::BoneAnimation, Engine::Resources::BaseAnimation)

namespace Engine::Resources
{
	using namespace Graphics;

	class ENGINE_BONEANIMATION_API BoneAnimation : public BaseAnimation
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(BoneAnimation)

		BoneAnimation(const AnimationPrimitive& primitive);

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		void          BindBone(const Weak<Bone>& bone_info);

		std::vector<Matrix> GetFrameAnimationDt(float dt);
		std::vector<Matrix> GetFrameAnimation(float time);

		RESOURCE_SELF_INFER_GETTER_DECL(BoneAnimation)

	protected:
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

#pragma once
#include <map>
#include "Source/Runtime/Resources/BaseAnimation/Public/BaseAnimation.h"
#include "AnimationPrimitive.h"

#include "BoneAnimation.generated.h"

namespace Engine::Resources
{
	using namespace Graphics;

	ECLASS(resource, serialize)
	class ENGINE_BONEANIMATION_API BoneAnimation : public BaseAnimation
	{
		GENERATE_BODY
	public:
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

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		BoneAnimation();

		EPROPERTY()
		AnimationPrimitive m_primitive_;
		
		EPROPERTY()
		MetadataPath       m_bone_path_;

		// non-serialized
		Strong<Bone>        m_bone_;
		float               m_evaluated_time_;
		std::vector<Matrix> m_evaluated_data_;
	};
}

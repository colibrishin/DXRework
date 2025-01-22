#pragma once
#include <map>
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "BoneAnimationPrimitive.h"

#include "Serialization.hpp"
#include "AnimationPrimitive.generated.h"

namespace Engine::Graphics 
{
	ECLASS()
	struct ENGINE_BONEANIMATION_API AnimationPrimitive
	{
		GENERATE_BODY
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

#pragma once
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

#include "BoneAnimationPrimitive.generated.h"

namespace Engine::Graphics 
{
	ECLASS()
	struct ENGINE_BASEANIMATION_API BoneAnimationPrimitive
	{
		GENERATE_BODY
	public:
		BoneAnimationPrimitive();

		void SetIndex(const int idx) noexcept;
		void AddPosition(const float time, const Vector3& position);
		void AddScale(const float time, const Vector3& scale);
		void AddRotation(const float time, const Quaternion& rotation);
		[[nodiscard]] int GetIndex() const noexcept;
		[[nodiscard]] Vector3 GetPosition(const float time) const;
		[[nodiscard]] Vector3 GetScale(const float time) const;
		[[nodiscard]] Quaternion GetRotation(const float time) const;

	private:
		int                                       bone_idx;
		std::vector<std::pair<float, Vector3>>    m_positions_{};
		std::vector<std::pair<float, Vector3>>    m_scales_{};
		std::vector<std::pair<float, Quaternion>> m_rotations_{};
	};
}
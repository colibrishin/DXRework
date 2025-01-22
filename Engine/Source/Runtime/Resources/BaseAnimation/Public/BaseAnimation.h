#pragma once
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "ResourceManager/Public/ResourceManager.h"

#include "BaseAnimation.generated.h"

namespace Engine::Graphics 
{
	struct ENGINE_BASEANIMATION_API BoneAnimationPrimitive
	{
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

		friend class boost::serialization::access;

		template <typename Archive>
		void serialize(Archive& ar, const unsigned int version) 
		{
			ar& bone_idx;
			ar& m_positions_;
			ar& m_scales_;
			ar& m_rotations_;
		}
	};
}

namespace Engine::Resources
{
	using namespace Graphics;

	ECLASS(resource)
	class ENGINE_BASEANIMATION_API BaseAnimation : public Abstracts::Resource
	{
		GENERATE_BODY
	public:
		BaseAnimation(const BoneAnimationPrimitive& primitive);

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		void OnDeserialized() override;
		void OnSerialized() override;

		void  SetTicksPerSecond(const float& ticks_per_second);
		void  SetDuration(const float& duration);
		float GetTicksPerSecond() const;
		float GetDuration() const;

		static float ConvertDtToFrame(const float& dt, float ticks_per_second);

	protected:
		friend class Components::Animator;
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

		BaseAnimation();

		EPROPERTY()
		float m_ticks_per_second_;
		
		EPROPERTY()
		float m_duration_;

		EPROPERTY()
		BoneAnimationPrimitive m_simple_primitive_;
	};
}
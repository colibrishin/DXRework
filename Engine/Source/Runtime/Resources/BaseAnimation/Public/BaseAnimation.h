#pragma once
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "ResourceManager/Public/ResourceManager.h"
#include "BoneAnimationPrimitive.h"

#include "BaseAnimation.generated.h"

namespace Engine::Resources
{
	using namespace Graphics;

	ECLASS(resource, serialize)
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
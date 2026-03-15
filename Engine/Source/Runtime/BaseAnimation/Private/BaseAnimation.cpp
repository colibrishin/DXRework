#include "BaseAnimation.h"
#include "ResourceManager.h"

namespace Engine::Resources
{
	BaseAnimation::BaseAnimation(const BoneAnimationPrimitive& primitive)
		: Resource(""),
		  m_ticks_per_second_(0),
		  m_duration_(0),
		  m_simple_primitive_(primitive) {}

	void BaseAnimation::PreUpdate(const float dt) {}

	void BaseAnimation::Update(const float dt) {}

	void BaseAnimation::FixedUpdate(const float dt) {}

	void BaseAnimation::PostUpdate(const float dt) {}

	void BaseAnimation::OnDeserialized()
	{
		Resource::OnDeserialized();
	}

	void BaseAnimation::OnSerialized()
	{
		Resource::OnSerialized();
	}

	void BaseAnimation::SetTicksPerSecond(const float& ticks_per_second)
	{
		m_ticks_per_second_ = ticks_per_second;
	}

	void BaseAnimation::SetDuration(const float& duration)
	{
		m_duration_ = duration;
	}

	float BaseAnimation::GetTicksPerSecond() const
	{
		return m_ticks_per_second_;
	}

	float BaseAnimation::GetDuration() const
	{
		return m_duration_;
	}

	void                             BaseAnimation::Load_INTERNAL() {}

	void BaseAnimation::Unload_INTERNAL() {}

	BaseAnimation::BaseAnimation()
		: Resource(""),
		  m_ticks_per_second_(0),
		  m_duration_(0) {}

	float BaseAnimation::ConvertDtToFrame(const float& dt, const float ticks_per_second)
	{
		return dt * ticks_per_second;
	}
}
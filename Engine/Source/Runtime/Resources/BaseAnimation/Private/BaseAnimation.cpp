#include "../Public/BaseAnimation.h"
#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Resources
{
	BaseAnimation::BaseAnimation(const BoneAnimationPrimitive& primitive)
		: Resource("", RES_T_BASE_ANIM),
		  m_ticks_per_second_(0),
		  m_duration_(0),
		  m_simple_primitive_(primitive) {}

	void BaseAnimation::PreUpdate(const float& dt) {}

	void BaseAnimation::Update(const float& dt) {}

	void BaseAnimation::FixedUpdate(const float& dt) {}

	void BaseAnimation::PostUpdate(const float& dt) {}

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

	boost::shared_ptr<BaseAnimation> BaseAnimation::Create(
		const std::string& name, const BoneAnimationPrimitive& primitive
	)
	{
		if (const auto check = Managers::ResourceManager::GetInstance().GetResource<BaseAnimation>(name).lock())
		{
			return check;
		}
		const auto obj = boost::make_shared<BaseAnimation>(primitive);
		Managers::ResourceManager::GetInstance().AddResource(name, obj);
		return obj;
	}

	void                             BaseAnimation::Load_INTERNAL() {}

	void BaseAnimation::Unload_INTERNAL() {}

	BaseAnimation::BaseAnimation()
		: Resource("", RES_T_BASE_ANIM),
		  m_ticks_per_second_(0),
		  m_duration_(0) {}

	float BaseAnimation::ConvertDtToFrame(const float& dt, const float ticks_per_second)
	{
		return dt * ticks_per_second;
	}
}

Engine::Graphics::BoneAnimationPrimitive::BoneAnimationPrimitive()
	: bone_idx(0) { }

void Engine::Graphics::BoneAnimationPrimitive::SetIndex(const int idx) noexcept
{
	bone_idx = idx;
}

void Engine::Graphics::BoneAnimationPrimitive::AddPosition(const float time, const Vector3& position)
{
	m_positions_.emplace_back(time, position);
}

void Engine::Graphics::BoneAnimationPrimitive::AddScale(const float time, const Vector3& scale)
{
	m_scales_.emplace_back(time, scale);
}

void Engine::Graphics::BoneAnimationPrimitive::AddRotation(const float time, const Quaternion& rotation)
{
	m_rotations_.emplace_back(time, rotation);
}

int Engine::Graphics::BoneAnimationPrimitive::GetIndex() const noexcept
{
	return bone_idx;
}

Vector3 Engine::Graphics::BoneAnimationPrimitive::GetPosition(const float time) const
{
	if (m_positions_.size() == 1)
	{
		return m_positions_[0].second;
	}

	for (int i = 0; i < m_positions_.size() - 1; ++i)
	{
		if (time < m_positions_[i + 1].first)
		{
			const auto& p0 = m_positions_[i];
			const auto& p1 = m_positions_[i + 1];

			const auto t = (time - p0.first) / (p1.first - p0.first);

			return Vector3::Lerp(p0.second, p1.second, t);
		}
	}

	return m_positions_.back().second;
}

Vector3 Engine::Graphics::BoneAnimationPrimitive::GetScale(const float time) const
{
	if (m_scales_.size() == 1)
	{
		return m_scales_[0].second;
	}

	for (size_t i = 0; i < m_scales_.size() - 1; ++i)
	{
		if (time < m_scales_[i + 1].first)
		{
			const auto& p0 = m_scales_[i];
			const auto& p1 = m_scales_[i + 1];

			const auto t = (time - p0.first) / (p1.first - p0.first);

			return Vector3::Lerp(p0.second, p1.second, t);
		}
	}

	return m_scales_.back().second;
}

Quaternion Engine::Graphics::BoneAnimationPrimitive::GetRotation(const float time) const
{
	for (size_t i = 0; i < m_rotations_.size() - 1; ++i)
	{
		if (time < m_rotations_[i + 1].first)
		{
			const auto& p0 = m_rotations_[i];
			const auto& p1 = m_rotations_[i + 1];

			const auto t    = (time - p0.first) / (p1.first - p0.first);
			const auto lerp = Quaternion::Slerp(p0.second, p1.second, t);
			Quaternion norm;
			lerp.Normalize(norm);

			return norm;
		}
	}

	return m_rotations_.back().second;
}

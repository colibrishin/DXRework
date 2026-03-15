#include "BoneAnimationPrimitive.h"

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

Engine::Vector3 Engine::Graphics::BoneAnimationPrimitive::GetPosition(const float time) const
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

Engine::Vector3 Engine::Graphics::BoneAnimationPrimitive::GetScale(const float time) const
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

Engine::Quaternion Engine::Graphics::BoneAnimationPrimitive::GetRotation(const float time) const
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

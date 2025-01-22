#include "AtlasAnimationPrimitive.h"
#include "AtlasAnimation.generated.h"

void Engine::AtlasAnimationPrimitive::Append(const AtlasFramePrimitive& frame)
{
	m_frames_.push_back(frame);
	m_total_duration_ += frame.Duration;
}

void Engine::AtlasAnimationPrimitive::SetTextureWidth(const UINT width)
{
	m_texture_width_ = width;
}

void Engine::AtlasAnimationPrimitive::SetTextureHeight(const UINT height)
{
	m_texture_height_ = height;
}

void Engine::AtlasAnimationPrimitive::SetUnitWidth(const UINT width)
{
	m_unit_width_ = width;
}

void Engine::AtlasAnimationPrimitive::SetUnitHeight(const UINT height)
{
	m_unit_height_ = height;
}

UINT Engine::AtlasAnimationPrimitive::GetTextureWidth() const noexcept
{
	return m_texture_width_;
}

UINT Engine::AtlasAnimationPrimitive::GetTextureHeight() const noexcept
{
	return m_texture_height_;
}

UINT Engine::AtlasAnimationPrimitive::GetUnitWidth() const noexcept
{
	return m_unit_width_;
}

UINT Engine::AtlasAnimationPrimitive::GetUnitHeight() const noexcept
{
	return m_unit_height_;
}

const Engine::AtlasFramePrimitive& Engine::AtlasAnimationPrimitive::GetFrame(
	const size_t idx
) const
{
	if (idx >= m_frames_.size())
	{
		return m_frames_.back();
	}

	return m_frames_[idx];
}

void Engine::AtlasAnimationPrimitive::GetFrame(const float frame, AtlasFramePrimitive& out) const
{
	float total_duration;
	GetTotalFrameDuration(total_duration);
	size_t frame_count;
	GetFrameCount(frame_count);

	float total = 0;

	for (size_t i = 0; i < frame_count; ++i)
	{
		total += m_frames_[i].Duration;

		if (frame < total)
		{
			out = m_frames_[i];
			return;
		}
	}

	out = m_frames_.back();
}

void Engine::AtlasAnimationPrimitive::GetFrameCount(size_t& count) const noexcept
{
	count = m_frames_.size();
}

void Engine::AtlasAnimationPrimitive::GetTotalFrameDuration(float& total) const noexcept
{
	total = m_total_duration_;
}

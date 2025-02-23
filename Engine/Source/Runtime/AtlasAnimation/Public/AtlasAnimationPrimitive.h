#pragma once
#include <filesystem>

#include "TypeLibrary.h"
#include "Serialization.hpp"
#include "AtlasFramePrimitive.h"

#include "AtlasAnimationPrimitive.generated.h"

namespace Engine
{
	ECLASS(serialize)
	struct ENGINE_ATLASANIMATION_API AtlasAnimationPrimitive
	{
		GENERATE_BODY
	public:
		void Append(const AtlasFramePrimitive& frame);

		void SetTextureWidth(const UINT width);

		void SetTextureHeight(const UINT height);

		void SetUnitWidth(const UINT width);

		void SetUnitHeight(const UINT height);

		[[nodiscard]] UINT GetTextureWidth() const noexcept;

		[[nodiscard]] UINT GetTextureHeight() const noexcept;

		[[nodiscard]] UINT GetUnitWidth() const noexcept;

		[[nodiscard]] UINT GetUnitHeight() const noexcept;

		[[nodiscard]] const AtlasFramePrimitive& GetFrame(const size_t idx) const;

		// Get atlas frame by duration.
		void __vectorcall GetFrame(const float frame, AtlasFramePrimitive& out) const;

		// Get atlas frame by index, for tight loop.
		void __vectorcall GetFrameCount(size_t& count) const noexcept;

		// Get total duration of the atlas animation, for tight loop.
		void __vectorcall GetTotalFrameDuration(float& total) const noexcept;

	private:
		EPROPERTY()
		UINT m_texture_width_ = 0;
		EPROPERTY()
		UINT m_texture_height_ = 0;
		EPROPERTY()
		UINT m_unit_width_ = 0;
		EPROPERTY()
		UINT m_unit_height_ = 0;
		EPROPERTY()
		float m_total_duration_ = 0;
		EPROPERTY()
		std::vector<AtlasFramePrimitive> m_frames_;
	};
}
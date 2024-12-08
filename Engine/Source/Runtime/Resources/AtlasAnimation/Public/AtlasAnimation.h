#pragma once
#include <filesystem>
#include "Source/Runtime/Resources/BaseAnimation/Public/BaseAnimation.h"

namespace Engine 
{
	struct ATLASANIMATION_API AtlasAnimationPrimitive
	{
	public:
		struct AtlasFramePrimitive
		{
			UINT  X;
			UINT  Y;
			UINT  Width;
			UINT  Height;
			float Duration;
		};

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
		UINT m_texture_width_ = 0;
		UINT m_texture_height_ = 0;

		UINT m_unit_width_ = 0;
		UINT m_unit_height_ = 0;

		float m_total_duration_ = 0;

		std::vector<AtlasFramePrimitive> m_frames_;
	};
}

namespace Engine::Resources
{
	using namespace Graphics;

	class ATLASANIMATION_API AtlasAnimation : public BaseAnimation
	{
	public:
		RESOURCE_T(RES_T_ATLAS_ANIM)

		AtlasAnimation(const AtlasAnimationPrimitive& primitive);

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnDeserialized() override;
		void OnSerialized() override;

		eResourceType GetResourceType() const override;

		void __vectorcall GetFrame(float dt, AtlasAnimationPrimitive::AtlasFramePrimitive& out) const;

		RESOURCE_SELF_INFER_GETTER_DECL(AtlasAnimation)

		[[nodiscard]] static AtlasAnimationPrimitive ParseXML(const std::filesystem::path& path);

		static boost::shared_ptr<AtlasAnimation> Create(
			const std::string& name, const std::filesystem::path& xml_path
		);

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

		AtlasAnimation() = default;

		std::string             m_xml_path_;
		AtlasAnimationPrimitive m_primitive_;
	};
}

#pragma once
#include <filesystem>
#include "Source/Runtime/Resources/BaseAnimation/Public/BaseAnimation.h"

namespace Engine 
{
	struct AtlasAnimationPrimitive
	{
	public:
		struct AtlasFramePrimitive
		{
			UINT  X;
			UINT  Y;
			UINT  Width;
			UINT  Height;
			float Duration;

		private:
			friend class boost::serialization::access;

			template <class Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				ar& X;
				ar& Y;
				ar& Width;
				ar& Height;
				ar& Duration;
			}
		};

		void Append(const AtlasFramePrimitive& frame)
		{
			m_frames_.push_back(frame);
			m_total_duration_ += frame.Duration;
		}

		void SetTextureWidth(const UINT width)
		{
			m_texture_width_ = width;
		}

		void SetTextureHeight(const UINT height)
		{
			m_texture_height_ = height;
		}

		void SetUnitWidth(const UINT width)
		{
			m_unit_width_ = width;
		}

		void SetUnitHeight(const UINT height)
		{
			m_unit_height_ = height;
		}

		[[nodiscard]] UINT GetTextureWidth() const noexcept
		{
			return m_texture_width_;
		}

		[[nodiscard]] UINT GetTextureHeight() const noexcept
		{
			return m_texture_height_;
		}

		[[nodiscard]] UINT GetUnitWidth() const noexcept
		{
			return m_unit_width_;
		}

		[[nodiscard]] UINT GetUnitHeight() const noexcept
		{
			return m_unit_height_;
		}

		[[nodiscard]] const AtlasFramePrimitive& GetFrame(const size_t idx) const
		{
			if (idx >= m_frames_.size())
			{
				return m_frames_.back();
			}

			return m_frames_[idx];
		}

		// Get atlas frame by duration.
		void __vectorcall GetFrame(const float frame, AtlasFramePrimitive& out) const
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

		// Get atlas frame by index, for tight loop.
		void __vectorcall GetFrameCount(size_t& count) const noexcept
		{
			count = m_frames_.size();
		}

		// Get total duration of the atlas animation, for tight loop.
		void __vectorcall GetTotalFrameDuration(float& total) const noexcept
		{
			total = m_total_duration_;
		}

	private:
		friend class boost::serialization::access;

		UINT m_texture_width_ = 0;
		UINT m_texture_height_ = 0;

		UINT m_unit_width_ = 0;
		UINT m_unit_height_ = 0;

		float m_total_duration_ = 0;

		std::vector<AtlasFramePrimitive> m_frames_;

		template <class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar& m_unit_width_;
			ar& m_unit_height_;
			ar& m_frames_;
			ar& m_texture_width_;
			ar& m_texture_height_;
			ar& m_total_duration_;
		}
	};
}

namespace Engine::Resources
{
	using namespace Graphics;

	class AtlasAnimation : public BaseAnimation
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

		[[nodiscard]] static AtlasAnimationPrimitive ParseXML(const boost::filesystem::path& path);

		static boost::shared_ptr<AtlasAnimation> Create(
			const std::string& name, const boost::filesystem::path& xml_path
		);

	protected:
		SERIALIZE_DECL

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

		AtlasAnimation() = default;

		std::string             m_xml_path_;
		AtlasAnimationPrimitive m_primitive_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::AtlasAnimation)

#include "../Public/AtlasAnimation.h"

#include <pugixml.hpp>
#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"
#include "Source/Runtime/Managers/StepTimer/Public/StepTimer.hpp"

namespace Engine::Resources
{
	AtlasAnimation::AtlasAnimation(const AtlasAnimationPrimitive& primitive)
		: m_primitive_(primitive) {}

	void AtlasAnimation::PreUpdate(const float& dt)
	{
		BaseAnimation::PreUpdate(dt);
	}

	void AtlasAnimation::Update(const float& dt)
	{
		BaseAnimation::Update(dt);
	}

	void AtlasAnimation::FixedUpdate(const float& dt)
	{
		BaseAnimation::FixedUpdate(dt);
	}

	void AtlasAnimation::PostUpdate(const float& dt)
	{
		BaseAnimation::PostUpdate(dt);
	}

	void AtlasAnimation::OnDeserialized()
	{
		BaseAnimation::OnDeserialized();
	}

	void AtlasAnimation::OnSerialized()
	{
		BaseAnimation::OnSerialized();

		if (!std::filesystem::exists(GetPrettyTypeName()))
		{
			std::filesystem::create_directory(GetPrettyTypeName());
		}

		// Backup the xml file
		const std::filesystem::path xml_path = m_xml_path_;
		const std::filesystem::path p        = GetPrettyTypeName() / xml_path.filename();

		if (m_xml_path_ == p.string())
		{
			return;
		}

		copy_file
				(
				 m_xml_path_, GetPrettyTypeName() / xml_path.filename(),
				 std::filesystem::copy_options::overwrite_existing
				);
		m_xml_path_ = (GetPrettyTypeName() / xml_path.filename()).string();
	}

	eResourceType AtlasAnimation::GetResourceType() const
	{
		return RES_T_ATLAS_ANIM;
	}

	void AtlasAnimation::GetFrame(const float dt, AtlasAnimationPrimitive::AtlasFramePrimitive& out) const
	{
		m_primitive_.GetFrame(ConvertDtToFrame(dt, GetTicksPerSecond()), out);
	}

	void AtlasAnimation::Load_INTERNAL()
	{
		BaseAnimation::Load_INTERNAL();
	}

	void AtlasAnimation::Unload_INTERNAL()
	{
		BaseAnimation::Unload_INTERNAL();
	}

	AtlasAnimationPrimitive AtlasAnimation::ParseXML(const std::filesystem::path& path)
	{
		pugi::xml_document doc;
		doc.load_file(path.c_str());

		const auto              root     = doc.root();
		const auto              anim_tag = root.child("AnimatedTexturePC");
		AtlasAnimationPrimitive primitive;

		const UINT width       = anim_tag.attribute("width").as_uint();
		const UINT height      = anim_tag.attribute("height").as_uint();
		const UINT unit_width  = anim_tag.attribute("actualWidth").as_uint();
		const UINT unit_height = anim_tag.attribute("actualHeight").as_uint();

		primitive.SetTextureHeight(width);
		primitive.SetTextureWidth(height);
		primitive.SetUnitHeight(unit_width);
		primitive.SetUnitWidth(unit_height);

		const auto frames = anim_tag.child("Frames");

		// For each FramePC
		for (const auto& child : frames.children())
		{
			// todo: proper conversion
			constexpr UINT tick = DX::StepTimer::TicksPerSecond;

			const float duration = static_cast<float>(child.attribute("duration").as_uint()) / tick;
			const auto  rect     = child.child("Rectangle");

			const UINT x = rect.attribute("x").as_uint();
			const UINT y = rect.attribute("y").as_uint();
			const UINT w = rect.attribute("w").as_uint();
			const UINT h = rect.attribute("h").as_uint();

			primitive.Append({x, y, w, h, duration});
		}

		return primitive;
	}

	inline boost::shared_ptr<AtlasAnimation> Engine::Resources::AtlasAnimation::Create(const std::string& name, const std::filesystem::path& xml_path)
	{
		if (const auto check = Managers::ResourceManager::GetInstance().GetResource<AtlasAnimation>(name).lock())
		{
			return check;
		}

		// Parse the xml file
		const auto& primitive = ParseXML(xml_path);
		const auto  obj = boost::make_shared<AtlasAnimation>(primitive);

		// Save primitive value for serialization and loading.
		obj->m_xml_path_ = xml_path.string();

		obj->SetTicksPerSecond(1.f);

		float duration = 0.f;
		primitive.GetTotalFrameDuration(duration);
		obj->SetDuration(duration);

		Managers::ResourceManager::GetInstance().AddResource(name, obj);
		return obj;
	}
}

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

const Engine::AtlasAnimationPrimitive::AtlasFramePrimitive& Engine::AtlasAnimationPrimitive::GetFrame(
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

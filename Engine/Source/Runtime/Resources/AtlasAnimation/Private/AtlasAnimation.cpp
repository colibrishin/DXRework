#include "../Public/AtlasAnimation.h"

#include "Source/Runtime/Managers/StepTimer/Public/StepTimer.hpp"
#include <pugixml.hpp>

SERIALIZE_IMPL
(
 Engine::Resources::AtlasAnimation,
 _ARTAG(_BSTSUPER(Engine::Resources::BaseAnimation))
 _ARTAG(m_xml_path_)
 _ARTAG(m_primitive_)
)

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

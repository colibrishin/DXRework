#include "pch.hpp"
#include "egFont.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include "egSerialization.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Resources::Font,
	_ARTAG(_BSTSUPER(Resource))
	_ARTAG(m_position_)
	_ARTAG(m_color_)
	_ARTAG(m_rotation_radian_)
	_ARTAG(m_scale_)
	_ARTAG(m_text_)
)

namespace Engine::Resources
{
	Font::Font(const std::filesystem::path& path) : Abstract::Resource(path, RESOURCE_PRIORITY_FONT), m_lazy_reload_(false), m_rotation_radian_(0), m_scale_(1)
	{
	}

	void Font::Initialize()
	{
	}

	void Font::PreUpdate(const float& dt)
	{
	}

	void Font::Update(const float& dt)
	{
	}

	void Font::PreRender(const float& dt)
	{
		if (m_lazy_reload_)
		{
			Unload();
			Load();
			m_lazy_reload_ = false;
		}
	}

	void Font::Render(const float& dt)
	{
		m_font_->DrawString(GetToolkitAPI().GetSpriteBatch(), m_text_.c_str(), m_position_, m_color_, m_rotation_radian_, Vector2::Zero, m_scale_);
	}

	void Font::FixedUpdate(const float& dt)
	{
	}

	void Font::PostRender(const float& dt)
	{
	}

	TypeName Font::GetVirtualTypeName() const
	{
		return typeid(Font).name();
	}

	void Font::Unload_INTERNAL()
	{
		m_font_.reset();
	}

	Font::Font(): Abstract::Resource("", RESOURCE_PRIORITY_FONT), m_rotation_radian_(0), m_scale_(1),
	              m_lazy_reload_(false)
	{
	}

	void Font::Load_INTERNAL()
	{
		m_font_ = std::make_unique<SpriteFont>(GetD3Device().GetDevice(), GetPath().c_str());
	}
}
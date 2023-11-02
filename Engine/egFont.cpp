#include "pch.hpp"
#include "egFont.hpp"

namespace Engine::Resources
{
	Font::Font(const std::filesystem::path& path) : Abstract::Resource(path, RESOURCE_PRIORITY_FONT), m_lazy_reload_(false), m_rotation_radian_(0), m_scale_(1)
	{
	}

	void Font::Initialize()
	{
	}

	void Font::PreUpdate()
	{
	}

	void Font::Update()
	{
	}

	void Font::PreRender()
	{
		if (m_lazy_reload_)
		{
			Unload();
			Load();
			m_lazy_reload_ = false;
		}
	}

	void Font::Render()
	{
		m_font_->DrawString(GetToolkitAPI().GetSpriteBatch(), m_text_.c_str(), m_position_, m_color_, m_rotation_radian_, Vector2::Zero, m_scale_);
	}

	void Font::FixedUpdate()
	{
	}

	void Font::Unload_INTERNAL()
	{
		m_font_.reset();
	}

	void Font::Load_INTERNAL()
	{
		m_font_ = std::make_unique<SpriteFont>(GetD3Device().GetDevice(), std::filesystem::absolute(GetPath()).c_str());
	}
}

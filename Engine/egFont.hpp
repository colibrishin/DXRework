#pragma once
#include "egD3Device.hpp"
#include "egResource.hpp"
#include "egToolkitAPI.hpp"

namespace Engine::Resources
{
	class Font : public Abstract::Resource
	{
	public:
		Font();
		~Font() override = default;

		void Initialize() override;

		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

		void SetText(const std::wstring& text) { m_text_ = text; }
		void SetPosition(const Vector2& position) { m_position_ = position; }
		void SetColor(const Vector4& color) { m_color_ = color; }
		void SetRotation(const float radian) { m_rotation_radian_ = radian; }
		void SetScale(const float scale) { m_scale_ = scale; }

		void ChangeFont(const std::filesystem::path& path) { SetPath(path); m_lazy_reload_ = true; }

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		bool m_lazy_reload_;

		Vector2 m_position_;
		Vector4 m_color_;
		float m_rotation_radian_;
		float m_scale_;
		std::wstring m_text_;

		std::unique_ptr<SpriteFont> m_font_;

	};

	inline Font::Font() : Abstract::Resource("", RESOURCE_PRIORITY_FONT), m_lazy_reload_(false), m_rotation_radian_(0), m_scale_(1)
	{
	}

	inline void Font::Initialize()
	{
	}

	inline void Font::PreUpdate()
	{
	}

	inline void Font::Update()
	{
	}

	inline void Font::PreRender()
	{
		if (m_lazy_reload_)
		{
			Unload_INTERNAL();
			Load_INTERNAL();
			m_lazy_reload_ = false;
		}
	}

	inline void Font::Render()
	{
		m_font_->DrawString(Graphic::ToolkitAPI::GetSpriteBatch(), m_text_.c_str(), m_position_, m_color_, m_rotation_radian_, Vector2::Zero, m_scale_);
	}

	inline void Font::Unload_INTERNAL()
	{
		m_font_.reset();
	}

	inline void Font::Load_INTERNAL()
	{
		m_font_ = std::make_unique<SpriteFont>(Graphic::D3Device::GetDevice(), std::filesystem::absolute(GetPath()).c_str());
	}
}

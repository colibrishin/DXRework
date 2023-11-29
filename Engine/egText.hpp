#pragma once
#include "egFont.hpp"
#include "egObject.hpp"

namespace Engine::Objects
{
	using WeakFontPtr = std::weak_ptr<Resources::Font>;
	class Text : public Abstract::Object
	{
	public:
		explicit Text(const WeakScene& initial_scene, const WeakFontPtr& font) : Object(initial_scene), m_rotation_radian_(0), m_scale_(1)
		{
			SetCulled(false);

			if (const auto font_ptr = font.lock())
			{
				AddResource(font_ptr);
			}
		}

		~Text() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;

		void SetText(const std::wstring& text) { m_text_ = text; }
		void SetPosition(const Vector2& position) { m_position_ = position; }
		void SetColor(const Vector4& color) { m_color_ = color; }
		void SetRotation(const float radian) { m_rotation_radian_ = radian; }
		void SetScale(const float scale) { m_scale_ = scale; }

	private:
		Vector2 m_position_;
		Vector4 m_color_;
		float m_rotation_radian_;
		float m_scale_;
		std::wstring m_text_;
	};

	inline void Text::Initialize()
	{
	}

	inline void Text::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);
	}

	inline void Text::Update(const float& dt)
	{
		Object::Update(dt);
	}

	inline void Text::PreRender(const float dt)
	{
		Object::PreRender(dt);
	}

	inline void Text::Render(const float dt)
	{
		if (const auto font = GetResource<Resources::Font>().lock())
		{
			font->SetText(m_text_);
			font->SetPosition(m_position_);
			font->SetColor(m_color_);
			font->SetRotation(m_rotation_radian_);
			font->SetScale(m_scale_);

			Object::Render(dt);

			font->SetText(L"");
			font->SetPosition({0.0f, 0.0f});
			font->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
			font->SetRotation(0);
			font->SetScale(1);
		}
	}
}

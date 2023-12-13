#pragma once
#include "egFont.hpp"
#include "egObject.hpp"
#include <boost/serialization/export.hpp>

namespace Engine::Objects
{
	class Text : public Abstract::Object
	{
	public:
		explicit Text(const WeakFont& font) : Object(), m_rotation_radian_(0), m_scale_(1)
		{
			SetCulled(false);

			if (const auto font_ptr = font.lock())
			{
				AddResource(font_ptr);
			}
		}

		~Text() override = default;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void OnDeserialized() override;

		void SetText(const std::string& text) { m_text_ = text; }
		void SetPosition(const Vector2& position) { m_position_ = position; }
		void SetColor(const Vector4& color) { m_color_ = color; }
		void SetRotation(const float radian) { m_rotation_radian_ = radian; }
		void SetScale(const float scale) { m_scale_ = scale; }

	protected:
		Text() : Object(), m_rotation_radian_(0), m_scale_(1) {}

	private:
		SERIALIZER_ACCESS

		Vector2 m_position_;
		Vector4 m_color_;
		float m_rotation_radian_;
		float m_scale_;
		std::string m_text_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Objects::Text)

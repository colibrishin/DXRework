#include "pch.h"
#include "egText.h"

#include "egResourceManager.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Objects::Text,
                       _ARTAG(_BSTSUPER(Object)) _ARTAG(m_position_)
                       _ARTAG(m_color_) _ARTAG(m_scale_) _ARTAG(m_text_))

namespace Engine::Objects
{
    Text::Text(const WeakFont& font)
    : Object(),
      m_rotation_radian_(0),
      m_scale_(1)
    {
        SetCulled(false);

        if (const auto font_ptr = font.lock())
        {
            AddResource(font_ptr);
        }
    }

    Text::~Text() = default;

    void Text::SetText(const std::string& text)
    {
        m_text_ = text;
    }

    void Text::SetPosition(const Vector2& position)
    {
        m_position_ = position;
    }

    void Text::SetColor(const Vector4& color)
    {
        m_color_ = color;
    }

    void Text::SetRotation(const float radian)
    {
        m_rotation_radian_ = radian;
    }

    void Text::SetScale(const float scale)
    {
        m_scale_ = scale;
    }

    Text::Text()
    : Object(),
      m_rotation_radian_(0),
      m_scale_(1) {}

    void Text::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    void Text::Update(const float& dt)
    {
        Object::Update(dt);
    }

    void Text::PreRender(const float& dt)
    {
        Object::PreRender(dt);
    }

    void Text::Render(const float& dt)
    {
        if (const auto font = GetResource<Resources::Font>().lock())
        {
            font->SetText(m_text_);
            font->SetPosition(m_position_);
            font->SetColor(m_color_);
            font->SetRotation(m_rotation_radian_);
            font->SetScale(m_scale_);

            Object::Render(dt);

            font->SetText("");
            font->SetPosition({0.0f, 0.0f});
            font->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
            font->SetRotation(0);
            font->SetScale(1);
        }
    }

    void Text::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }

    void Text::OnDeserialized()
    {
        Object::OnDeserialized();
    }
} // namespace Engine::Objects

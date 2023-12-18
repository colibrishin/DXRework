#include "pch.h"
#include "egText.h"

#include "egResourceManager.hpp"

SERIALIZER_ACCESS_IMPL(
                       Engine::Objects::Text,
                       _ARTAG(_BSTSUPER(Object)) _ARTAG(m_position_)
                       _ARTAG(m_color_) _ARTAG(m_scale_) _ARTAG(m_text_))

namespace Engine::Objects
{
    Text::Text(const WeakFont& font)
    : Object(DEF_OBJ_T_TEXT),
      m_rotation_radian_(0),
      m_scale_(1),
      m_font_(font.lock())
    {
        SetCulled(false);
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
    : Object(DEF_OBJ_T_TEXT),
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
        m_font_->SetText(m_text_);
        m_font_->SetPosition(m_position_);
        m_font_->SetColor(m_color_);
        m_font_->SetRotation(m_rotation_radian_);
        m_font_->SetScale(m_scale_);

        Object::Render(dt);
        m_font_->Render(dt);

        m_font_->SetText("");
        m_font_->SetPosition({0.0f, 0.0f});
        m_font_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
        m_font_->SetRotation(0);
        m_font_->SetScale(1);
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

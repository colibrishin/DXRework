#include "TextRenderer.h"
#include "TextRenderer.generated.h"

#include "Font.h"

#if WITH_EDITOR
#include "UIHelpersResourceManager.h"
#endif

void Engine::Components::TextRenderer::PreUpdate(const float dt)
{
}

void Engine::Components::TextRenderer::Update(const float dt)
{
    if ( m_loaded_font_ )
    {
        m_loaded_font_->GetPrimitive().Render(
            m_text_.data(), 
            m_position_, 
            m_color_, 
            m_rotation_rad_, 
            m_scale_);
    }
}

void Engine::Components::TextRenderer::FixedUpdate(const float dt)
{
}

#if WITH_EDITOR
void Engine::Components::TextRenderer::OnUIUpdate(UIContext* const parent, const float dt)
{
    if (parent)
    {
        RenderComponent::OnUIUpdate(parent, dt);
        {
            IUIAPI&       ui = s_uia.GetInterface();
            static std::string font_name;
            if (m_loaded_font_)
            {
                font_name = m_loaded_font_->GetName();
            }
            else
            {
                font_name = "";
            }
            *parent |= ui.NewLabelAndText( this, "Font", { "Font", font_name, false } );
            ( *parent |= ui.NewButton( this, "FontSelectionButton", { "Select Font..." } ) ).SetFunction( [this]()
            {
                m_b_font_dialog_ = !m_b_font_dialog_;
            } );

            *parent |= ui.NewLabelAndText( this, "Text", { "Text", m_text_, true } );
            *parent |= ui.NewLabelAndVec2( this, "Position", { "Position", &m_position_.x, 0.1f, 0, 0, true } );
            *parent |= ui.NewLabelAndVec4( this, "Color", { "Color", &m_color_.x, 0.1f, 0, 1, true } );
            *parent |= ui.NewLabelAndFloat( this, "Rotation", { "Rotation", m_rotation_rad_, 0, 0, 1, true } );
            *parent |= ui.NewLabelAndVec2( this, "Scale", { "Scale", &m_scale_.x, 0.1f, 0, 0, true } );
        }

        if (m_b_font_dialog_)
        {
            if (Weak<Engine::Abstracts::Resource> resource_to_load;
                UIHelpers::SingleResourceSelectionDialogInclusion<TextRenderer, Resources::Font>(
                    GetSharedPtr<TextRenderer>(),
                    resource_to_load))
            {
                if (const Strong<Resources::Font>& font = Cast<Resources::Font>(resource_to_load))
                {
                    SetFont(font);
                }

                m_b_font_dialog_ = false;
            }
        }
    }
}
#endif

void Engine::Components::TextRenderer::OnDeserialized()
{
    if (const Strong<Resources::Font>& font = Resources::Font::GetByMetadataPath(m_font_meta_path_).lock())
    {
        m_loaded_font_ = font;
    }
}

void Engine::Components::TextRenderer::OnSerialized()
{
    if (m_loaded_font_)
    {
        Serializer::Serialize(m_loaded_font_->GetName(), m_loaded_font_);
        m_font_meta_path_ = m_loaded_font_->GetMetadataPath();
    }
}

void Engine::Components::TextRenderer::SetFont(const Weak<Resources::Font>& font)
{
    if (const Strong<Resources::Font>& locked = font.lock())
    {
        m_loaded_font_ = locked;
        m_font_meta_path_ = locked->GetMetadataPath();
    }
}

void Engine::Components::TextRenderer::SetText(const std::string_view text)
{
    m_text_ = text;
}

void Engine::Components::TextRenderer::SetPosition(const Vector2& position)
{
    m_position_ = position;
}

void Engine::Components::TextRenderer::SetColor(const Color& color)
{
    m_color_ = color;
}

void Engine::Components::TextRenderer::SetRotation(const float radian)
{
    m_rotation_rad_ = radian;
}

void Engine::Components::TextRenderer::SetScale(const Vector2& scale)
{
    m_scale_ = scale;
}

Engine::eComponentUpdatePriorities Engine::Components::TextRenderer::GetUpdatePriority() const
{
    return COM_PRIORITY_RENDER;
}

#pragma once
#include "RenderComponent.h"

#include "TextRenderer.generated.h"

namespace Engine::Components
{
    ECLASS(component, serialize)
    class ENGINE_TEXTRENDERER_API TextRenderer : public RenderComponent
    {
        GENERATE_BODY

        void PreUpdate(const float dt) override;
        void Update(const float dt) override;
        void FixedUpdate(const float dt) override;

#if WITH_EDITOR
        void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

        void OnDeserialized() override;
        void OnSerialized() override;

        void SetFont(const Weak<Resources::Font>& font);
        void SetText(const std::string_view text);
        void SetPosition(const Vector2& position);
        void SetColor(const Color& color);
        void SetRotation(const float radian);
        void SetScale(const Vector2& scale);

        eComponentUpdatePriorities GetUpdatePriority() const override;

    private:
        using RenderComponent::RenderComponent;
        friend struct ConstructorAccess;

#if WITH_EDITOR
        bool m_b_font_dialog_ = false;
#endif

        EPROPERTY()
        std::filesystem::path m_font_meta_path_;
        EPROPERTY()
        std::string m_text_;
        EPROPERTY()
        Vector2 m_position_{ 0.f, 0.f };
        EPROPERTY()
        Color m_color_ { 1.f, 1.f, 1.f, 1.f };
        EPROPERTY()
        float m_rotation_rad_ = 0.f;
        EPROPERTY()
        Vector2 m_scale_ = { 1.f, 1.f };

        Strong<Resources::Font> m_loaded_font_;
    };
}
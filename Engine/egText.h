#pragma once
#include <boost/serialization/export.hpp>
#include "egFont.h"
#include "egObject.hpp"

namespace Engine::Objects
{
    class Text : public Abstract::Object
    {
    public:
        OBJECT_T(DEF_OBJ_T_TEXT)

        explicit Text(const WeakFont& font);

        ~Text() override;

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void OnDeserialized() override;

        void SetText(const std::string& text);
        void SetPosition(const Vector2& position);
        void SetColor(const Vector4& color);
        void SetRotation(const float radian);
        void SetScale(const float scale);

    protected:
        Text();

    private:
        SERIALIZER_ACCESS

        Vector2     m_position_;
        Vector4     m_color_;
        float       m_rotation_radian_;
        float       m_scale_;
        std::string m_text_;
        StrongFont  m_font_;
    };
} // namespace Engine::Objects

BOOST_CLASS_EXPORT_KEY(Engine::Objects::Text)

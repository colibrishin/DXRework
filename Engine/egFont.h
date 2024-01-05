#pragma once
#include "egD3Device.hpp"
#include "egResource.h"
#include "egToolkitAPI.h"

namespace Engine::Resources
{
    class Font : public Abstract::Resource
    {
    public:
        RESOURCE_T(RES_T_FONT)

        Font(const std::filesystem::path& path);
        ~Font() override = default;

        void Initialize() override;

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void PostUpdate(const float& dt) override;
        void Render(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostRender(const float& dt) override;

        void SetText(const std::string& text);
        void SetPosition(const Vector2& position);
        void SetColor(const Vector4& color);
        void SetRotation(const float radian);
        void SetScale(const float scale);
        void ChangeFont(const std::filesystem::path& path);

        RESOURCE_SELF_INFER_GETTER(Font)
        RESOURCE_SELF_INFER_CREATE(Font)

    protected:
        Font(); // for serialization
        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;

    private:
        SERIALIZER_ACCESS

        Vector2     m_position_;
        Vector4     m_color_;
        float       m_rotation_radian_;
        float       m_scale_;
        std::string m_text_;

        bool                        m_lazy_reload_;
        std::unique_ptr<SpriteFont> m_font_;
    };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Font)

#pragma once
#include <bitset>

#include "egCommon.hpp"
#include "egObject.hpp"
#include "egRenderPipeline.h"
#include "egTransform.h"

namespace Engine::Objects
{
    class Light final : public Abstract::Object
    {
    public:
        INTERNAL_OBJECT_CHECK_CONSTEXPR(DEF_OBJ_T_LIGHT)

        Light();

        ~Light() override;

        void SetColor(Vector4 color);

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

        void OnDeserialized() override;

        Color GetColor() const
        {
            return m_color_;
        }

    private:
        SERIALIZER_ACCESS
        Color m_color_;
    };
} // namespace Engine::Objects

BOOST_CLASS_EXPORT_KEY(Engine::Objects::Light)

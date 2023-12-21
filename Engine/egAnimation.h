#pragma once
#include "egCommon.hpp"
#include "egResource.h"

namespace Engine::Resources
{
    class Animation : public Abstract::Resource
    {
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_ANIM);

        Animation(const AnimationPrimitive& anim);
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

        RESOURCE_SELF_INFER_GETTER(Animation)
    protected:
        SERIALIZER_ACCESS
        Animation();

        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;

    private:
        AnimationPrimitive m_primitive_;
        ComPtr<ID3D11Buffer> m_animation_buffer_;

    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Animation);
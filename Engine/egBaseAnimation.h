#pragma once

namespace Engine::Resources
{
    class BaseAnimation : public Abstract::Resource
    {
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_BASE_ANIM)

        BaseAnimation(const BoneAnimationPrimitive& primitive);

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void PostUpdate(const float& dt) override;

        void SetFrame(const float& dt);
        void SetTicksPerSecond(const float& ticks_per_second);
        void SetDuration(const float& duration);

        RESOURCE_SELF_INFER_GETTER(BaseAnimation)

    protected:
        SERIALIZER_ACCESS

        void  Load_INTERNAL() override;
        void  Unload_INTERNAL() override;

        BaseAnimation();

        float ConvertDtToFrame(const float& dt) const;

        float m_current_frame_;
        float m_ticks_per_second_;
        float m_duration_;

        BoneAnimationPrimitive m_simple_primitive_;

    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::BaseAnimation)
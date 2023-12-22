#pragma once

namespace Engine::Resources
{
    class Animation : public Abstract::Resource
    {
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_ANIM)

        Animation(const AnimationPrimitive& primitive);

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

        void                            BindBone(const WeakBone & bone_info);
        void SetFrame(const float& dt);

    protected:
        void  Load_INTERNAL() override;
        void  Unload_INTERNAL() override;

    private:
        float ConvertDtToFrame(const float& dt) const;
        std::vector<BoneTransformElement> GetFrameAnimation() const;

        float m_current_frame_;

        AnimationPrimitive m_primitive_;
        StrongBone          m_bone_;

        ComPtr<ID3D11ShaderResourceView> m_animation_buffer_;

    };
}
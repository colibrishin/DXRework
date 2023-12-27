#pragma once
#include "egBaseAnimation.h"

namespace Engine::Resources
{
    class BoneAnimation : public BaseAnimation
    {
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_BONE_ANIM)

        BoneAnimation(const AnimationPrimitive& primitive);

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void PostUpdate(const float& dt) override;

        void RenderEnd();

        void BindBone(const WeakBone& bone_info);
        eResourceType GetResourceType() const override;

        std::vector<BoneTransformElement> GetBoneTransform(const float dt) const;

        RESOURCE_SELF_INFER_GETTER(BoneAnimation)

    protected:
        SERIALIZER_ACCESS

        void  Load_INTERNAL() override;
        void  Unload_INTERNAL() override;

    private:
        BoneAnimation();

        AnimationPrimitive m_primitive_;
        StrongBone          m_bone_;

        std::vector<BoneTransformElement> GetFrameAnimation(const float dt) const;

        ComPtr<ID3D11ShaderResourceView> m_animation_buffer_;


    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::BoneAnimation)
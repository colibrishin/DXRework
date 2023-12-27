#pragma once
#include "egBaseAnimation.h"
#include "egDXAnimCommon.hpp"

namespace Engine::Resources
{
    using namespace Engine::Graphics;

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

        void BindBone(const WeakBone& bone_info);
        eResourceType GetResourceType() const override;

        RESOURCE_SELF_INFER_GETTER(BoneAnimation)

    protected:
        SERIALIZER_ACCESS

        void  Load_INTERNAL() override;
        void  Unload_INTERNAL() override;

    private:
        BoneAnimation();

        std::vector<BoneTransformElement> GetFrameAnimation(const float dt) const;

        AnimationPrimitive m_primitive_;
        StrongBone          m_bone_;

        ComPtr<ID3D11ShaderResourceView> m_animation_buffer_;

    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::BoneAnimation)
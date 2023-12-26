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
        void PostUpdate(const float& dt) override;

        void BindBone(const WeakBone& bone_info);
        void SetFrame(const float& dt);

        RESOURCE_SELF_INFER_GETTER(Animation)

        static inline boost::shared_ptr<Animation> Create(const std::string& name, const AnimationPrimitive& primitive)
        {
            if (const auto check = GetResourceManager().GetResource<Animation>(name).lock())
            {
                return check;
            }

            const auto obj = boost::make_shared<Animation>(primitive);
            obj->m_b_bone_animation_ = false;
            GetResourceManager().AddResource(name, obj);
            return obj;
        }

    protected:
        SERIALIZER_ACCESS

        void  Load_INTERNAL() override;
        void  Unload_INTERNAL() override;

    private:
        Animation();

        float ConvertDtToFrame(const float& dt) const;
        std::vector<BoneTransformElement> GetFrameAnimation() const;

        float m_current_frame_;
        bool  m_b_bone_animation_;

        AnimationPrimitive m_primitive_;
        StrongBone          m_bone_;

        ComPtr<ID3D11ShaderResourceView> m_animation_buffer_;

    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Animation)
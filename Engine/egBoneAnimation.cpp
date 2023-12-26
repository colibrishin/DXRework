#include "pch.h"
#include "egBoneAnimation.h"

#include "egBone.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Resources::BoneAnimation,
                       _ARTAG(_BSTSUPER(BaseAnimation))
                       _ARTAG(m_primitive_)
                       _ARTAG(m_bone_)
                       _ARTAG(m_current_frame_))

namespace Engine::Resources
{
    BoneAnimation::BoneAnimation(const AnimationPrimitive& primitive)
    : BaseAnimation(),
      m_primitive_(primitive) {}

    void BoneAnimation::PreUpdate(const float& dt) {}

    void BoneAnimation::Update(const float& dt) {}

    void BoneAnimation::FixedUpdate(const float& dt) {}

    void BoneAnimation::PreRender(const float& dt) {}

    void BoneAnimation::Render(const float& dt)
    {
        SetFrame(m_current_frame_ + dt);
        auto animation_per_bone = GetFrameAnimation();

        GetD3Device().CreateStructuredShaderResource<BoneTransformElement>(animation_per_bone.size(), animation_per_bone.data(), m_animation_buffer_.ReleaseAndGetAddressOf());

        GetRenderPipeline().BindResource(SR_ANIMATION, SHADER_VERTEX, m_animation_buffer_.Get());
    }

    void BoneAnimation::PostRender(const float& dt) {}

    void BoneAnimation::PostUpdate(const float& dt) {}

    void BoneAnimation::BindBone(const WeakBone& bone_info)
    {
        if (const auto locked = bone_info.lock())
        {
            m_bone_ = locked;
        }
    }

    eResourceType BoneAnimation::GetResourceType() const
    {
        return RES_T_BONE_ANIM;
    }

    std::vector<BoneTransformElement> BoneAnimation::GetFrameAnimation() const
    {
        const auto anim_time = ConvertDtToFrame(m_current_frame_);
        std::vector<BoneTransformElement> rtn;
        std::vector<Matrix> memo;

        memo.resize(m_primitive_.GetBoneCount());

        for (int i = 0; i < m_primitive_.GetBoneCount(); ++i)
        {
            BoneTransformElement bfa;
            const BoneAnimationPrimitive* bone_animation = m_primitive_.GetBoneAnimation(i);
            const BonePrimitive* bone           = m_bone_->GetBone(i);
            const BonePrimitive* parent         = m_bone_->GetBoneParent(i);

            const auto position = bone_animation->GetPosition(anim_time);
            const auto rotation = bone_animation->GetRotation(anim_time);
            const auto scale = bone_animation->GetScale(anim_time);

            const Matrix vertex_transform = Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(position);

            Matrix parent_transform = Matrix::Identity;

            if (parent)
            {
                parent_transform = memo[parent->GetIndex()];
            }

            const Matrix node_transform = vertex_transform;

            const Matrix global_transform = node_transform * parent_transform;
            memo[bone->GetIndex()] = global_transform;

            const auto final_transform = bone->GetInvBindPose() * global_transform  * m_primitive_.GetGlobalInverseTransform();
            bfa.transform = final_transform.Transpose();
            rtn.push_back(bfa);
        }

        return rtn;
    }

    void BoneAnimation::Load_INTERNAL()
    {
        SetDuration(m_primitive_.GetDuration());
        SetTicksPerSecond(m_primitive_.GetTicksPerSecond());
    }

    void BoneAnimation::Unload_INTERNAL() {}

    BoneAnimation::BoneAnimation()
    : BaseAnimation(),
      m_primitive_() { }
}

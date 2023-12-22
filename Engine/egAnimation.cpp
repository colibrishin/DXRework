#include "pch.h"
#include "egAnimation.h"

#include "egBone.h"

namespace Engine::Resources
{
    Animation::Animation(const AnimationPrimitive& primitive)
    : Resource("", RES_T_ANIM),
      m_current_frame_(0),
      m_primitive_(primitive) {}

    void Animation::PreUpdate(const float& dt) {}

    void Animation::Update(const float& dt) {}

    void Animation::FixedUpdate(const float& dt) {}

    void Animation::PreRender(const float& dt) {}

    void Animation::Render(const float& dt)
    {
        auto animation_per_bone = GetFrameAnimation();

        GetD3Device().CreateStructuredShaderResource<BoneFrameAnimation>(animation_per_bone.size(), animation_per_bone.data(), m_animation_buffer_.ReleaseAndGetAddressOf());

        GetRenderPipeline().BindResource(SR_ANIMATION, SHADER_VERTEX, m_animation_buffer_.Get());
    }

    void Animation::PostRender(const float& dt) {}

    void Animation::BindBone(const WeakBone& bone_info)
    {
        if (const auto locked = bone_info.lock())
        {
            m_bone_ = locked;
        }
    }

    std::vector<BoneFrameAnimation> Animation::GetFrameAnimation() const
    {
        const auto anim_time = ConvertDtToFrame(m_current_frame_);
        std::vector<Matrix> memo;
        std::vector<BoneFrameAnimation> rtn;

        memo.resize(m_primitive_.GetBoneCount(), Matrix::Identity);

        for (int i = 0; i < m_primitive_.GetBoneCount(); ++i)
        {
            BoneFrameAnimation bfa;
            const BoneAnimation& bone_animation = m_primitive_.GetBoneAnimation(i);
            const BonePrimitive bone = m_bone_->GetBone(i);

            Matrix transform = Matrix::Identity;

            const auto position = bone_animation.GetPosition(anim_time);
            const auto rotation = bone_animation.GetRotation(anim_time);
            const auto scale = bone_animation.GetScale(anim_time) / 100;

            transform = Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(position);
            memo.at(i) = transform;

            if (bone.parent_idx != -1)
            {
                transform = memo.at(bone.parent_idx) * transform;
            }
            
            const auto inv_bind_pose = bone.offset;

            const auto final_transform = m_primitive_.global_inverse_transform * transform * inv_bind_pose;
            bfa.transform = final_transform.Transpose();
            rtn.push_back(bfa);
        }

        return rtn;
    }

    void Animation::SetFrame(const float& dt)
    {
        m_current_frame_ = dt;
    }

    void Animation::Load_INTERNAL() {}

    void Animation::Unload_INTERNAL() {}

    float Animation::ConvertDtToFrame(const float& dt) const
    {
        return std::fmod(dt * m_primitive_.ticks_per_second, m_primitive_.duration);
    }
}

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
        SetFrame(m_current_frame_ + dt);
        auto animation_per_bone = GetFrameAnimation();

        GetD3Device().CreateStructuredShaderResource<BoneTransformElement>(animation_per_bone.size(), animation_per_bone.data(), m_animation_buffer_.ReleaseAndGetAddressOf());

        GetRenderPipeline().BindResource(SR_ANIMATION, SHADER_VERTEX, m_animation_buffer_.Get());
    }

    void Animation::PostRender(const float& dt) {}

    void Animation::PostUpdate(const float& dt) {}

    void Animation::BindBone(const WeakBone& bone_info)
    {
        if (const auto locked = bone_info.lock())
        {
            m_bone_ = locked;
        }
    }

    std::vector<BoneTransformElement> Animation::GetFrameAnimation() const
    {
        const auto anim_time = ConvertDtToFrame(m_current_frame_);
        std::vector<BoneTransformElement> rtn;
        std::vector<Matrix> memo;

        memo.resize(m_primitive_.GetBoneCount());

        for (int i = 0; i < m_primitive_.GetBoneCount(); ++i)
        {
            BoneTransformElement bfa;
            const BoneAnimation* bone_animation = m_primitive_.GetBoneAnimation(i);
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

    void Animation::SetFrame(const float& dt)
    {
        if (m_current_frame_ >= m_primitive_.GetDuration() / m_primitive_.GetTicksPerSecond())
        {
            m_current_frame_ = 0;
        }

        m_current_frame_ = dt;
    }

    void Animation::Load_INTERNAL() {}

    void Animation::Unload_INTERNAL() {}

    float Animation::ConvertDtToFrame(const float& dt) const
    {
        return std::fmod(dt * m_primitive_.GetTicksPerSecond(), m_primitive_.GetDuration());
    }
}

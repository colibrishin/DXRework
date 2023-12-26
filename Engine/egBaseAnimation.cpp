#include "pch.h"
#include "egBaseAnimation.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Resources::BaseAnimation,
                       _ARTAG(_BSTSUPER(Resource))
                       _ARTAG(m_current_frame_)
                       _ARTAG(m_ticks_per_second_)
                       _ARTAG(m_duration_)
                       _ARTAG(m_simple_primitive_))

namespace Engine::Resources
{
    BaseAnimation::BaseAnimation(const BoneAnimationPrimitive& primitive)
    : Resource("", RES_T_BASE_ANIM),
      m_current_frame_(0),
      m_ticks_per_second_(0),
      m_duration_(0),
      m_simple_primitive_(primitive) {}

    void BaseAnimation::PreUpdate(const float& dt) {}

    void BaseAnimation::Update(const float& dt) {}

    void BaseAnimation::FixedUpdate(const float& dt) {}

    void BaseAnimation::PreRender(const float& dt) {}

    void BaseAnimation::Render(const float& dt)
    {
        const auto time = ConvertDtToFrame(dt);

        const auto position = m_simple_primitive_.GetPosition(time);
        const auto rotation = m_simple_primitive_.GetRotation(time);
        const auto scale = m_simple_primitive_.GetScale(time);
        const auto transform = Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(position);

        auto object_transform = GetRenderPipeline().GetWorldMatrix();
        object_transform.world *= transform;
        GetRenderPipeline().SetWorldMatrix(object_transform);
    }

    void BaseAnimation::PostRender(const float& dt) {}

    void BaseAnimation::PostUpdate(const float& dt) {}

    void BaseAnimation::SetFrame(const float& dt)
    {
        m_current_frame_ = dt;
    }

    void BaseAnimation::SetTicksPerSecond(const float& ticks_per_second)
    {
        m_ticks_per_second_ = ticks_per_second;
    }

    void BaseAnimation::SetDuration(const float& duration)
    {
        m_duration_ = duration;
    }

    void BaseAnimation::Load_INTERNAL() {}

    void BaseAnimation::Unload_INTERNAL() {}

    BaseAnimation::BaseAnimation()
    : Resource("", RES_T_BASE_ANIM),
      m_current_frame_(0),
      m_ticks_per_second_(0),
      m_duration_(0) {}

    float BaseAnimation::ConvertDtToFrame(const float& dt) const
    {
        return std::fmod(dt * m_ticks_per_second_, m_duration_);
    }
}

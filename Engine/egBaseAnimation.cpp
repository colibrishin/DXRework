#include "pch.h"
#include "egBaseAnimation.h"

#include "egDXAnimCommon.hpp"

SERIALIZER_ACCESS_IMPL
(
 Engine::Resources::BaseAnimation,
 _ARTAG(_BSTSUPER(Resource))
 _ARTAG(m_ticks_per_second_)
 _ARTAG(m_duration_)
 _ARTAG(m_simple_primitive_)
)

namespace Engine::Resources
{
  BaseAnimation::BaseAnimation(const BoneAnimationPrimitive& primitive)
    : Resource("", RES_T_BASE_ANIM),
      m_ticks_per_second_(0),
      m_duration_(0),
      m_simple_primitive_(primitive) {}

  void BaseAnimation::PreUpdate(const float& dt) {}

  void BaseAnimation::Update(const float& dt) {}

  void BaseAnimation::FixedUpdate(const float& dt) {}

  void BaseAnimation::PreRender(const float& dt) {}

  void BaseAnimation::Render(const float& dt) {}

  void BaseAnimation::PostRender(const float& dt) {}

  void BaseAnimation::PostUpdate(const float& dt) {}

  void BaseAnimation::SetTicksPerSecond(const float& ticks_per_second) { m_ticks_per_second_ = ticks_per_second; }

  void BaseAnimation::SetDuration(const float& duration) { m_duration_ = duration; }

  float BaseAnimation::GetTicksPerSecond() const { return m_ticks_per_second_; }

  float BaseAnimation::GetDuration() const { return m_duration_; }

  void BaseAnimation::Load_INTERNAL() {}

  void BaseAnimation::Unload_INTERNAL() {}

  BaseAnimation::BaseAnimation()
    : Resource("", RES_T_BASE_ANIM),
      m_ticks_per_second_(0),
      m_duration_(0) {}

  float BaseAnimation::ConvertDtToFrame(const float& dt, const float ticks_per_second, const float duration)
  {
    return dt * ticks_per_second;
  }
}

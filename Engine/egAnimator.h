#pragma once
#include "egComponent.h"

namespace Engine::Components
{
  class Animator final : public Abstract::Component
  {
  public:
    COMPONENT_T(COM_T_ANIMATOR)

    Animator(const WeakObject& owner);
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;

    void OnSerialized() override;
    void OnDeserialized() override;
    void OnImGui() override;

    void SetAnimation(UINT idx);

    UINT  GetAnimation() const;
    float GetFrame() const;
    float GetDt() const;

  private:
    SERIALIZE_DECL
    Animator();

    void UpdateTransform(const StrongTransform& tr, const StrongBaseAnimation& anim) const;

    template <typename T>
    void ResetIfTimer(const boost::shared_ptr<T>& anim)
    {
      if (anim->ConvertDtToFrame(m_total_dt_, anim->GetTicksPerSecond(), anim->GetDuration()) >= anim->GetDuration())
      {
        m_current_frame_ = 0.0f;
        m_total_dt_ = 0.0f;
      }
    }

    template <typename T>
    void UpdateTimer(const boost::shared_ptr<T>& anim)
    {
      m_current_frame_ = anim->ConvertDtToFrame(m_total_dt_, anim->GetTicksPerSecond(), anim->GetDuration());
    }

    UINT  m_animation_id_;
    float m_current_frame_;
    float m_total_dt_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::Animator)

#pragma once
#include "egManager.hpp"
#include "egPDepthTexture.h"
#include "egVelocityTexture.h"

namespace Engine::Manager::Graphics
{
  class MotionBlur : public Abstract::Singleton<MotionBlur>
  {
  public:
    explicit MotionBlur(SINGLETON_LOCK_TOKEN) {}

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void OnDeserialized() override;

  private:
    friend struct SingletonDeleter;
    ~MotionBlur() override = default;

    Resources::PDepthTexture m_previous_depth_;
    Resources::VelocityTexture m_velocity_texture_;
    StrongShader m_motion_blur_shader_;

  };
}

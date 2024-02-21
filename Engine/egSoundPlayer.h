#pragma once
#include "egCommon.hpp"
#include "egComponent.h"

namespace Engine::Components
{
  class SoundPlayer final : public Abstract::Component
  {
  public:
    COMPONENT_T(COMP_T_SOUND_PLAYER)

    SoundPlayer(const WeakObject& owner);
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void SetSound(const StrongSound& sound);
    void OnDeserialized() override;
    void OnImGui() override;

    void PlaySound();
    void PlaySoundLoop();

    void StopSound();

  private:
    SERIALIZER_ACCESS
    SoundPlayer();

    LocalResourceID m_sound_id_;

    // non-serialized
    StrongSound m_sound_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::SoundPlayer)

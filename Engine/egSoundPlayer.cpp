#include "pch.h"
#include "egSoundPlayer.h"

#include "egImGuiHeler.hpp"
#include "egResourceManager.hpp"
#include "egSound.h"

SERIALIZER_ACCESS_IMPL
(
 Engine::Components::SoundPlayer,
 _ARTAG(_BSTSUPER(Abstract::Component))
 _ARTAG(m_sound_name_)
)

namespace Engine::Components
{
  SoundPlayer::SoundPlayer(const WeakObject& owner)
    : Component(COMP_T_SOUND_PLAYER, owner) {}

  void SoundPlayer::PreUpdate(const float& dt) {}

  void SoundPlayer::Update(const float& dt)
  {
    if (m_sound_ == nullptr) { return; }

    if (!m_sound_->IsPlaying(GetOwner())) { return; }

    m_sound_->UpdatePosition(GetOwner());
  }

  void SoundPlayer::FixedUpdate(const float& dt) {}

  void SoundPlayer::PostUpdate(const float& dt) { Component::PostUpdate(dt); }

  void SoundPlayer::SetSound(const StrongSound& sound)
  {
    m_sound_      = sound;
    m_sound_name_ = sound->GetName();
  }

  void SoundPlayer::OnDeserialized()
  {
    Component::OnDeserialized();

    if (m_sound_name_.empty()) { return; }

    m_sound_ = GetResourceManager().GetResource<Resources::Sound>(m_sound_name_).lock();
  }

  void SoundPlayer::OnImGui()
  {
    Component::OnImGui();
    TextDisabled("Sound name", m_sound_name_);

    if (ImGui::BeginDragDropTarget())
    {
      if (const auto payload = ImGui::AcceptDragDropPayload("RESOURCE"))
      {
        const auto& resource = *static_cast<StrongResource*>(payload->Data);

        if (resource->GetResourceType() == RES_T_SOUND)
        {
          SetSound(resource->GetSharedPtr<Resources::Sound>());
        }
      }
      ImGui::EndDragDropTarget();
    }
  }

  void SoundPlayer::PlaySound() { m_sound_->Play(GetOwner()); }

  void SoundPlayer::PlaySoundLoop() { m_sound_->PlayLoop(GetOwner()); }

  void SoundPlayer::StopSound() { m_sound_->Stop(GetOwner()); }

  SoundPlayer::SoundPlayer()
    : Component(COMP_T_SOUND_PLAYER, {}) {}
}

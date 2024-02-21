#include "pch.h"
#include "egSoundPlayer.h"

#include "egImGuiHeler.hpp"
#include "egResourceManager.hpp"
#include "egSound.h"

SERIALIZER_ACCESS_IMPL
(
 Engine::Components::SoundPlayer,
 _ARTAG(_BSTSUPER(Abstract::Component))
 _ARTAG(m_sound_id_)
)

namespace Engine::Components
{
  SoundPlayer::SoundPlayer(const WeakObject& owner)
    : Component(COMP_T_SOUND_PLAYER, owner),
      m_sound_id_(g_invalid_id) {}

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
    m_sound_id_   = sound->GetLocalID();
  }

  void SoundPlayer::OnDeserialized()
  {
    Component::OnDeserialized();

    if (m_sound_id_ == g_invalid_id) { return; }

    m_sound_ = GetResourceManager().GetResource<Resources::Sound>(m_sound_id_).lock();
  }

  void SoundPlayer::OnImGui()
  {
    Component::OnImGui();
    lldDisabled("Sound ID", m_sound_id_);

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

  void SoundPlayer::PlaySound()
  {
    if (m_sound_)
    {
      m_sound_->Play(GetOwner());
    }
  }

  void SoundPlayer::PlaySoundLoop()
  {
    if (m_sound_)
    {
      m_sound_->PlayLoop(GetOwner());
    }
  }

  void SoundPlayer::StopSound()
  {
    if (m_sound_)
    {
      m_sound_->Stop(GetOwner());
    }
  }

  SoundPlayer::SoundPlayer()
    : Component(COMP_T_SOUND_PLAYER, {}),
      m_sound_id_(g_invalid_id) {}
}

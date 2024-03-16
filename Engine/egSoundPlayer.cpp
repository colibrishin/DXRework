#include "pch.h"
#include "egSoundPlayer.h"

#include "egImGuiHeler.hpp"
#include "egResourceManager.hpp"
#include "egSound.h"

SERIALIZE_IMPL
(
 Engine::Components::SoundPlayer,
 _ARTAG(_BSTSUPER(Abstract::Component))
 _ARTAG(m_sound_meta_path_)
)

namespace Engine::Components
{
  COMP_CLONE_IMPL(SoundPlayer)

  SoundPlayer::SoundPlayer(const WeakObjectBase& owner)
    : Component(COM_T_SOUND_PLAYER, owner) {}

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
    m_sound_meta_path_   = sound->GetMetadataPath().string();
  }

  void SoundPlayer::OnSerialized()
  {
    Serializer::Serialize(m_sound_->GetName(), m_sound_);
    m_sound_meta_path_ = m_sound_->GetMetadataPath().string();
  }

  void SoundPlayer::OnDeserialized()
  {
    Component::OnDeserialized();

    if (const auto sound = Resources::Sound::GetByMetadataPath(m_sound_meta_path_).lock())
    {
      SetSound(sound);
    }
  }

  void SoundPlayer::OnImGui()
  {
    Component::OnImGui();
    TextDisabled("Sound Metadata", m_sound_meta_path_);

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
    : Component(COM_T_SOUND_PLAYER, {}) {}
}

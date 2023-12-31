#include "pch.h"
#include "egSound.h"
#include "egDebugger.hpp"
#include "egObject.hpp"
#include "egRigidbody.h"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Resources::Sound,
                       _ARTAG(_BSTSUPER(Resource)) _ARTAG(m_mode_)
                       _ARTAG(m_min_distance_) _ARTAG(m_max_distance_))

namespace Engine::Resources
{
    Sound::Sound(const std::filesystem::path& path)
    : Resource(path, RES_T_SOUND),
      m_mode_(FMOD_3D),
      m_min_distance_(0.f),
      m_max_distance_(3.f) {}

    Sound::~Sound() = default;

    Sound::Sound()
    : Resource("", RES_T_SOUND),
      m_mode_(FMOD_3D),
      m_min_distance_(0.f),
      m_max_distance_(3.f) {}

    void Sound::CommitDistance() const
    {
        if (m_sound_)
        {
            m_sound_->set3DMinMaxDistance(m_min_distance_, m_max_distance_);
        }
    }

    void Sound::Initialize() {}

    void Sound::PreUpdate(const float& dt) {}

    void Sound::Update(const float& dt) {}

    void Sound::FixedUpdate(const float& dt) {}

    void Sound::PreRender(const float& dt) {}

    void Sound::Render(const float& dt) {}

    void Sound::PostRender(const float& dt) {}

    void Sound::PostUpdate(const float& dt) {}

    void Sound::Play_INTERNAL(const WeakObject& origin)
    {
        FMOD_VECTOR pos{};
        FMOD_VECTOR vel{};

        if (const auto tr =
                origin.lock()->GetComponent<Components::Transform>().lock())
        {
            pos = {tr->GetWorldPosition().x, tr->GetWorldPosition().y, tr->GetWorldPosition().z};
        }

        if (const auto rb =
                origin.lock()->GetComponent<Components::Rigidbody>().lock())
        {
            vel = {
                rb->GetLinearMomentum().x, rb->GetLinearMomentum().y,
                rb->GetLinearMomentum().z
            };
        }

        m_sound_->setMode(m_mode_);
        GetToolkitAPI().PlaySound(m_sound_, pos, vel, &m_channel_map_[origin]);
    }

    void Sound::Play(const WeakObject& origin)
    {
        m_mode_ |= FMOD_LOOP_OFF;
        m_mode_ &= ~FMOD_LOOP_NORMAL;
        Play_INTERNAL(origin);
    }

    void Sound::PlayLoop(const WeakObject& origin)
    {
        m_mode_ &= ~FMOD_LOOP_OFF;
        m_mode_ |= FMOD_LOOP_NORMAL;
        m_sound_->setMode(m_mode_);
        Play(origin);
    }

    bool Sound::IsPlaying(const WeakObject& origin)
    {
        bool is_playing = false;
        m_channel_map_[origin]->isPlaying(&is_playing);
        return is_playing;
    }

    void Sound::Stop(const WeakObject& origin)
    {
        GetToolkitAPI().StopSound(m_sound_, &m_channel_map_[origin]);
        m_channel_map_.erase(origin);
    }

    void Sound::StopLoop(const WeakObject& origin)
    {
        m_mode_ |= FMOD_LOOP_OFF;
        m_mode_ &= ~FMOD_LOOP_NORMAL;
        m_sound_->setMode(FMOD_3D);
        Stop(origin);
    }

    void Sound::UpdatePosition(const WeakObject& origin)
    {
        FMOD_VECTOR pos{};
        FMOD_VECTOR vel{};

        if (const auto tr = origin.lock()->GetComponent<Components::Transform>().lock())
        {
            pos = {tr->GetWorldPosition().x, tr->GetWorldPosition().y, tr->GetWorldPosition().z};
        }
        if (const auto rb = origin.lock()->GetComponent<Components::Rigidbody>().lock())
        {
            vel = {
                rb->GetLinearMomentum().x, rb->GetLinearMomentum().y,
                rb->GetLinearMomentum().z
            };
        }

        m_channel_map_[origin]->set3DAttributes(&pos, &vel);
    }

    void Sound::SetRollOff(const FMOD_MODE& roll_off) const
    {
        if (m_sound_)
        {
            if (!(roll_off ^ FMOD_3D_LINEARROLLOFF) ||
                !(roll_off ^ FMOD_3D_INVERSETAPEREDROLLOFF) ||
                !(roll_off ^ FMOD_3D_LINEARSQUAREROLLOFF))
            {
                m_sound_->setMode(roll_off);
            }
            else
            {
                GetDebugger().Log("Invalid roll off mode given.");
            }
        }
    }

    void Sound::SetMinDistance(const float& min_distance)
    {
        m_min_distance_ = min_distance;
        CommitDistance();
    }

    void Sound::SetMaxDistance(const float& max_distance)
    {
        m_max_distance_ = max_distance;
        CommitDistance();
    }

    void Sound::Load_INTERNAL()
    {
        GetToolkitAPI().LoadSound(&m_sound_, GetPath().generic_string());
        CommitDistance();
    }

    void Sound::Unload_INTERNAL()
    {
        m_sound_->release();
        m_sound_ = nullptr;
    }
} // namespace Engine::Resources

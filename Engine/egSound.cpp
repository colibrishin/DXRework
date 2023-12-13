#include "pch.hpp"
#include "egSound.hpp"
#include "egObject.hpp"
#include "egTransform.hpp"
#include "egRigidbody.hpp"
#include "egDebugger.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Resources::Sound,
	_ARTAG(_BSTSUPER(Resource))
	_ARTAG(m_mode_)
	_ARTAG(m_min_distance_)
	_ARTAG(m_max_distance_))

namespace Engine::Resources
{
	void Sound::Initialize()
	{
	}

	void Sound::PreUpdate(const float& dt)
	{
	}

	void Sound::Update(const float& dt)
	{
	}

	void Sound::FixedUpdate(const float& dt)
	{
	}

	void Sound::PreRender(const float& dt)
	{
	}

	void Sound::Render(const float& dt)
	{
	}

	void Sound::PostRender(const float& dt)
	{
	}

	TypeName Sound::GetVirtualTypeName() const
	{
		return typeid(Sound).name();
	}

	void Sound::Play_INTERNAL(const WeakObject& origin)
	{
		FMOD_VECTOR pos{};
		FMOD_VECTOR vel{};

		if (const auto tr = origin.lock()->GetComponent<Component::Transform>().lock())
		{
			pos = {tr->GetPosition().x, tr->GetPosition().y, tr->GetPosition().z};
		}

		if (const auto rb = origin.lock()->GetComponent<Component::Rigidbody>().lock())
		{
			vel = {rb->GetLinearMomentum().x, rb->GetLinearMomentum().y, rb->GetLinearMomentum().z};
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
				GetDebugger().Log(L"Invalid roll off mode given.");
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
}
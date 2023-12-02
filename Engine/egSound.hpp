#pragma once
#include "egToolkitAPI.hpp"

namespace Engine
{
	inline const FMOD_VECTOR g_fmod_forward = { 0, 0, -1.f };
	inline const FMOD_VECTOR g_fmod_up = { 0, 1, 0.f };
}

namespace Engine::Resources
{
	class Sound : public Abstract::Resource
	{
	public:
		explicit Sound(const std::string& path) : Resource(path, RESOURCE_PRIORITY_SOUND), m_mode_(FMOD_3D),
												m_min_distance_(0.f), m_max_distance_(3.f)
		{
		}

		~Sound() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;

		void Play(const WeakObject& origin);
		void PlayLoop(const WeakObject& origin);
		bool IsPlaying(const WeakObject& origin);
		void Stop(const WeakObject& origin);
		void StopLoop(const WeakObject& origin);

		void SetRollOff(const FMOD_MODE& roll_off) const
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
		void SetMinDistance(const float& min_distance)
		{
			m_min_distance_ = min_distance;
			CommitDistance();
		}
		void SetMaxDistance(const float& max_distance)
		{
			m_max_distance_ = max_distance;
			CommitDistance();
		}

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		friend class boost::serialization::access;
		void CommitDistance() const
		{
			if (m_sound_)
			{
				m_sound_->set3DMinMaxDistance(m_min_distance_, m_max_distance_);
			}
		}
		void Play_INTERNAL(const WeakObject& origin);

		FMOD::Sound* m_sound_ = nullptr;
		std::map<WeakObject, FMOD::Channel*, WeakObjComparer> m_channel_map_;
		FMOD_MODE m_mode_;

		float m_min_distance_;
		float m_max_distance_;

	};

	inline void Sound::Initialize()
	{
	}

	inline void Sound::PreUpdate(const float& dt)
	{
	}

	inline void Sound::Update(const float& dt)
	{
	}

	inline void Sound::FixedUpdate(const float& dt)
	{
	}

	inline void Sound::PreRender(const float dt)
	{
	}

	inline void Sound::Render(const float dt)
	{
	}

	inline void Sound::Play_INTERNAL(const WeakObject& origin)
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

	inline void Sound::Play(const WeakObject& origin)
	{
		m_mode_ |= FMOD_LOOP_OFF;
		m_mode_ &= ~FMOD_LOOP_NORMAL;
		Play_INTERNAL(origin);
	}

	inline void Sound::PlayLoop(const WeakObject& origin)
	{
		m_mode_ &= ~FMOD_LOOP_OFF;
		m_mode_ |= FMOD_LOOP_NORMAL;
		m_sound_->setMode(m_mode_);
		Play(origin);
	}

	inline bool Sound::IsPlaying(const WeakObject& origin)
	{
		bool is_playing = false;
		m_channel_map_[origin]->isPlaying(&is_playing);
		return is_playing;
	}

	inline void Sound::Stop(const WeakObject& origin)
	{
		GetToolkitAPI().StopSound(m_sound_, &m_channel_map_[origin]);
		m_channel_map_.erase(origin);
	}

	inline void Sound::StopLoop(const WeakObject& origin)
	{
		m_mode_ |= FMOD_LOOP_OFF;
		m_mode_ &= ~FMOD_LOOP_NORMAL;
		m_sound_->setMode(FMOD_3D);
		Stop(origin);
	}

	inline void Sound::Load_INTERNAL()
	{
		GetToolkitAPI().LoadSound(&m_sound_, GetPath().generic_string());
		CommitDistance();
	}

	inline void Sound::Unload_INTERNAL()
	{
		m_sound_->release();
		m_sound_ = nullptr;
	}
}

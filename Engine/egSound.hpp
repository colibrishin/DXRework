#pragma once
#include "egToolkitAPI.hpp"

namespace Engine
{
	inline FMOD_VECTOR g_fmod_forward_ = { 0, 0, -1.f };
	inline FMOD_VECTOR g_fmod_up_ = { 0, 1, 0.f };
}

namespace Engine::Resources
{
	class Sound : public Abstract::Resource
	{
	public:
		Sound(const std::string& path) : Resource(path, RESOURCE_PRIORITY_SOUND)
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

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		FMOD::Sound* m_sound_ = nullptr;
		std::map<WeakObject, FMOD::Channel*, WeakObjComparer> m_channel_map_;

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

	inline void Sound::Play(const WeakObject& origin)
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

		GetToolkitAPI().PlaySound(m_sound_, pos, vel, &m_channel_map_[origin]);
	}

	inline void Sound::PlayLoop(const WeakObject& origin)
	{
		m_sound_->setMode(FMOD_LOOP_NORMAL);
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
		m_sound_->setMode(FMOD_3D | FMOD_LOOP_OFF);
		Stop(origin);
	}

	inline void Sound::Load_INTERNAL()
	{
		GetToolkitAPI().LoadSound(&m_sound_, GetPath().generic_string());
		m_sound_->set3DMinMaxDistance(0.f, 3.f);
	}

	inline void Sound::Unload_INTERNAL()
	{
		m_sound_->release();
	}
}

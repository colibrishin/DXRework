#pragma once
#include "egToolkitAPI.hpp"
#include "egResource.hpp"

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
		TypeName GetVirtualTypeName() const final;

		void Play(const WeakObject& origin);
		void PlayLoop(const WeakObject& origin);
		bool IsPlaying(const WeakObject& origin);
		void Stop(const WeakObject& origin);
		void StopLoop(const WeakObject& origin);

		void SetRollOff(const FMOD_MODE& roll_off) const;
		void SetMinDistance(const float& min_distance);
		void SetMaxDistance(const float& max_distance);

	protected:
		Sound() : Resource("", RESOURCE_PRIORITY_SOUND), m_mode_(FMOD_3D), m_min_distance_(0.f), m_max_distance_(3.f)
		{
		}

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		SERIALIZER_ACCESS

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
		std::map<WeakObject, FMOD::Channel*, WeakComparer<Abstract::Object>> m_channel_map_;
		FMOD_MODE m_mode_;

		float m_min_distance_;
		float m_max_distance_;

	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Sound)
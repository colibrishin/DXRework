#pragma once
#include "Resource.h"
#include "ResourceManager.h"
#include "ISoundAPI.h"

#include "Sound.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_SOUND_API Sound : public Abstracts::Resource
	{
		GENERATE_BODY

		Sound(const Sound& other);
		Sound& operator=(const Sound& other);

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PostUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;
		void OnSerialized() override;

		void Play(const Weak<Components::Transform>& origin, const Vector3& velocity, bool loop);
		bool IsPlaying(const Weak<Components::Transform>& transform);
		void Stop(const Weak<Components::Transform>& transform);
		void StopLoop(const Weak<Components::Transform>& transform);
		void UpdatePositionAndVelocity(const Weak<Components::Transform>& transform, const Vector3& velocity);
		
		void SetRollOff(const UINT roll_off) const;
		void SetMinDistance(const float min_distance);
		void SetMaxDistance(const float max_distance);

	protected:
		Sound() : Resource("") {};

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		explicit Sound(const std::filesystem::path& path) : Resource(path) {};

		std::unordered_map<Weak<Components::Transform>, SoundChannelID> m_assigned_ids_;

		struct SoundDeleter 
		{
			void operator()(ISound* ptr) const
			{
				g_sound_accessor.GetInterface().ReleaseSound( ptr );
			}
		};

		Unique<ISound, SoundDeleter> m_primitive_;

		EPROPERTY()
		UINT m_roll_off_ = 0;
		EPROPERTY()
		float m_min_distance_ = 1.f;
		EPROPERTY()
		float m_max_distance_ = 10000.f;
	};
}

#pragma once
#include "TypeLibrary.h"
#undef PlaySound

namespace Engine 
{
	typedef UINT SoundChannelID;
	constexpr size_t g_max_sound_channel = 32;

	struct ISound;

	struct ENGINE_CORE_API ISoundAPI
	{
		virtual ~ISoundAPI() = default;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual void Update() = 0;
		virtual ISound* NewSound( const std::filesystem::path& path ) = 0;
		virtual void ReleaseSound( ISound* primitive ) = 0;
		virtual void UpdatePosition( const SoundChannelID id, const Vector3& position ) = 0;
		virtual void UpdatePosition( const SoundChannelID id, const Vector3& position, const Vector3& velocity ) = 0;
		virtual bool PlaySound( const ISound* sound, const Vector3& position, const Vector3& velocity, bool loop, SoundChannelID& id ) = 0;
		virtual bool StopSound( const SoundChannelID id ) = 0;
		virtual void StopLoop( const ISound* sound, const SoundChannelID id ) = 0;
	};

	struct ENGINE_CORE_API ISoundAPIAccessor
	{
		ISoundAPI& GetInterface()
		{
			return *s_interface_;
		}

		template <typename T>
		void SetInterface()
		{
			if (!s_interface_)
			{
				s_interface_ = std::make_unique<T>();
				s_interface_->Initialize();
			}
		}

		void Shutdown()
		{
			if (s_interface_)
			{
				s_interface_->Shutdown();
				s_interface_ = nullptr;
			}
		}

	private:
		Unique<ISoundAPI> s_interface_;
	};

	extern ENGINE_CORE_API ISoundAPIAccessor g_sound_accessor;

	struct ENGINE_CORE_API ISound
	{
		virtual ~ISound() {}

		virtual void SetMinDistance( float value ) = 0;
		virtual void SetMaxDistance( float value ) = 0;
		virtual void SetRollOff( const UINT value ) = 0;
	};
}


#pragma once
#include "TypeLibrary/Public/TypeLibrary.h"
#undef PlaySound

namespace Engine 
{
	typedef UINT SoundChannelID;
	constexpr size_t g_max_sound_channel = 32;

	struct SoundPrimitive;

	struct ENGINE_CORE_API SoundInterface
	{
		virtual ~SoundInterface() = default;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual void Update() = 0;
		virtual SoundPrimitive* NewSound( const std::filesystem::path& path ) = 0;
		virtual void ReleaseSound( SoundPrimitive* primitive ) = 0;
		virtual void UpdatePosition( const SoundChannelID id, const Vector3& position ) = 0;
		virtual void UpdatePosition( const SoundChannelID id, const Vector3& position, const Vector3& velocity ) = 0;
		virtual bool PlaySound( const SoundPrimitive* sound, const Vector3& position, const Vector3& velocity, bool loop, SoundChannelID& id ) = 0;
		virtual bool StopSound( const SoundChannelID id ) = 0;
		virtual void StopLoop( const SoundPrimitive* sound, const SoundChannelID id ) = 0;
	};

	struct ENGINE_CORE_API SoundInterfaceAccessor
	{
		static SoundInterface& GetInterface()
		{
			return *s_interface_;
		}

		template <typename T>
		static void SetInterface()
		{
			if (!s_interface_)
			{
				s_interface_ = std::make_unique<T>();
				s_interface_->Initialize();
			}
		}

		static void Shutdown()
		{
			if (!s_interface_)
			{
				s_interface_->Shutdown();
				s_interface_ = nullptr;
			}
		}

	private:
		static Unique<SoundInterface> s_interface_;
	};

	struct ENGINE_CORE_API SoundPrimitive
	{
		virtual ~SoundPrimitive() {}

		virtual void SetMinDistance( float value ) = 0;
		virtual void SetMaxDistance( float value ) = 0;
		virtual void SetRollOff( const UINT value ) = 0;
	};
}


#pragma once
#include "ISoundAPI.h"
#include "Allocator.h"
#include <Public/fmod.hpp>
#include <Public/fmod_common.h>
#include <magic_enum/magic_enum.hpp>

#include "FMODSoundInterface.generated.h"

namespace Engine
{
	struct ENGINE_FMODSOUNDINTERFACE_API FMODSoundPrimitive : public ISound
	{
        virtual ~FMODSoundPrimitive();

		void SetMinDistance(float value) override;
		void SetMaxDistance(float value) override;
		void SetRollOff(const UINT value) override;
		FMOD_MODE GetMode() const;
		void SetMode(const FMOD_MODE mode) const;

		FMOD::Sound** GetAddressOf()
		{
			return &m_sound_;
		}

		FMOD::Sound* Get() const
		{
			return m_sound_;
		}

	private:
		void SetNativeSound(FMOD::Sound* sound)
		{
			m_sound_ = sound;
		}

		float m_min_distance_ = 1.f;
		float m_max_distance_ = 10000.f;
		FMOD::Sound* m_sound_ = nullptr;
	};

	ECLASS()
    struct ENGINE_FMODSOUNDINTERFACE_API FMODSoundInterface : ISoundAPI
    {
        GENERATE_BODY
        ~FMODSoundInterface() override;

        void    Initialize() override;
        void    Shutdown() override;
        void    Update() override;
        ISound* NewSound( const std::filesystem::path& path ) override;
        void    ReleaseSound( ISound* primitive ) override;
        void    UpdatePosition( const SoundChannelID id, const Vector3& position ) override;
        void    UpdatePosition( const SoundChannelID id, const Vector3& position, const Vector3& velocity ) override;
        bool    PlaySound( const ISound*   sound,
                           const Vector3&  position,
                           const Vector3&  velocity,
                           bool            loop,
                           SoundChannelID& id ) override;
        bool    StopSound( const SoundChannelID id ) override;
        void    StopLoop( const ISound* sound, const SoundChannelID id ) override;

    private:
        FMOD::System*         m_audio_engine_         = nullptr;
        FMOD::ChannelGroup*   m_master_channel_group_ = nullptr;
        FMOD::ChannelControl* m_channel_control_      = nullptr;

        std::array<FMOD::Channel*, g_max_sound_channel>  m_channel_map_{};
        aligned_vector<FMODSoundPrimitive> m_instanced_primitives_{};
    };
}
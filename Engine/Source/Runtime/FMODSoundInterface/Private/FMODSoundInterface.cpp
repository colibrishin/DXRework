#include "FMODSoundInterface.h"
#include "FMODSoundInterface.generated.h"

static bool HandleFMODResult(FMOD_RESULT result)
{
	if (result != FMOD_OK)
	{
#if WITH_DEBUG || WITH_EDITOR
		std::string exception_output = std::format( "Error thrown from fmod: {}", magic_enum::enum_name<FMOD_RESULT>( result ) );
		OutputDebugStringA( exception_output.c_str() );
#endif
		return false;
	}

	return true;
}

Engine::FMODSoundInterface::~FMODSoundInterface()
{
}

void Engine::FMODSoundInterface::Initialize()
{
	HandleFMODResult( System_Create( &m_audio_engine_ ) );
	HandleFMODResult( m_audio_engine_->init( g_max_sound_channel, FMOD_INIT_NORMAL, nullptr ));
	HandleFMODResult( m_audio_engine_->createChannelGroup( "Master", &m_master_channel_group_ ) );
	HandleFMODResult( m_master_channel_group_->setVolume( 0.5f ) );
}

void Engine::FMODSoundInterface::Shutdown()
{
	m_audio_engine_->update();
	m_audio_engine_->release();
    m_instanced_primitives_.clear();
}

void Engine::FMODSoundInterface::Update()
{
	m_audio_engine_->update();
}

Engine::ISound* Engine::FMODSoundInterface::NewSound( const std::filesystem::path& path )
{
    // todo: fix allocation logic
    Engine::FMODSoundPrimitive new_sound;

    if ( HandleFMODResult( m_audio_engine_->createSound(
                 path.generic_string().c_str(), FMOD_3D | FMOD_3D_LINEARROLLOFF, nullptr, new_sound.GetAddressOf() ) ) )
    {
        return &m_instanced_primitives_.emplace_back( new_sound );
    }

    return nullptr;
}

void Engine::FMODSoundInterface::ReleaseSound( ISound* primitive )
{
    if ( auto* ptr = static_cast<FMODSoundPrimitive*>( primitive ) )
    {
        std::ranges::remove_if( m_instanced_primitives_, [ & ]( FMODSoundPrimitive elem ) { return ptr == &elem; } );
    }
}

void Engine::FMODSoundInterface::UpdatePosition(const SoundChannelID id, const Vector3& position)
{
	if ( const auto& channel = m_channel_map_[ id ] )
	{
		FMOD_VECTOR pos{ position.x, position.y, position.z };
		m_channel_map_[ id ]->set3DAttributes(&pos, nullptr);
	}
}

void Engine::FMODSoundInterface::UpdatePosition(const SoundChannelID id, const Vector3& position, const Vector3& velocity)
{
	if ( const auto& channel = m_channel_map_[ id ] )
	{
		FMOD_VECTOR pos{ position.x, position.y, position.z };
		FMOD_VECTOR vel{ velocity.x, velocity.y, velocity.z };

		m_channel_map_[ id ]->set3DAttributes(&pos, &vel);
	}
}

bool Engine::FMODSoundInterface::PlaySound(const ISound* sound, const Vector3& position, const Vector3& velocity, bool loop, SoundChannelID& id)
{
	if ( auto* primitive = static_cast<const FMODSoundPrimitive*>( sound ) )
	{
		const auto& it = std::ranges::find( m_channel_map_, nullptr );
		if ( it == m_channel_map_.end() )
		{
			return false;
		}
		
		if ( HandleFMODResult( m_audio_engine_->playSound(
			primitive->Get(), 
			m_master_channel_group_, 
			false, 
			&(*it) ) ) )
		{
			id = static_cast<SoundChannelID>(std::distance(m_channel_map_.begin(), it));
			return true;
		}
	}
	

    return false;
}

bool Engine::FMODSoundInterface::StopSound(const SoundChannelID id)
{
	FMOD::Channel* channel = m_channel_map_[id];

	if (channel)
	{
		channel->stop();
		m_channel_map_[id] = nullptr;

		return true;
	}

	return false;
}

void Engine::FMODSoundInterface::StopLoop(const ISound* sound, const SoundChannelID id)
{
	if ( auto* primitive = static_cast<const FMODSoundPrimitive*>(sound) )
	{
		FMOD_MODE mode = primitive->GetMode();
		mode |= FMOD_LOOP_NORMAL;
		mode &= ~FMOD_LOOP_OFF;
		primitive->SetMode( mode );
	}
}

Engine::FMODSoundPrimitive::~FMODSoundPrimitive()
{
    if ( m_sound_ )
    {
        m_sound_->release();
    }
}

void Engine::FMODSoundPrimitive::SetMinDistance( float value )
{
	m_min_distance_ = value;
	HandleFMODResult( m_sound_->set3DMinMaxDistance( m_min_distance_, m_max_distance_ ) );
}

void Engine::FMODSoundPrimitive::SetMaxDistance(float value)
{
	m_max_distance_ = value;
	HandleFMODResult( m_sound_->set3DMinMaxDistance( m_min_distance_, m_max_distance_ ) );
}

void Engine::FMODSoundPrimitive::SetRollOff(const UINT value)
{
}

FMOD_MODE Engine::FMODSoundPrimitive::GetMode() const
{
	FMOD_MODE mode{};

	if (m_sound_)
	{
		HandleFMODResult(m_sound_->getMode(&mode));
	}

	return mode;
}

inline void Engine::FMODSoundPrimitive::SetMode(const FMOD_MODE mode) const
{
	if (m_sound_)
	{
		HandleFMODResult(m_sound_->setMode(mode));
	}
}

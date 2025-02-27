#include "Sound.h"
#include "Sound.generated.h"

#include "Transform.h"

Engine::Resources::Sound::Sound(const Sound& other) : Resource(other)
{
	m_roll_off_ = other.m_roll_off_;
	m_min_distance_ = other.m_min_distance_;
	m_max_distance_ = other.m_max_distance_;
}

Engine::Resources::Sound& Engine::Resources::Sound::operator=(const Sound& other)
{
	m_roll_off_ = other.m_roll_off_;
	m_min_distance_ = other.m_min_distance_;
	m_max_distance_ = other.m_max_distance_;
	return *this;
}

void Engine::Resources::Sound::PreUpdate(const float dt)
{
}

void Engine::Resources::Sound::Update(const float dt)
{
	for ( auto it = m_assigned_ids_.begin(); it != m_assigned_ids_.end(); )
	{
		const auto& [ transform, id ] = *it;
		
		if ( const Strong<Components::Transform>& locked = transform.lock() )
		{
			ISoundAPI &si = s_sa.GetInterface();
			si.UpdatePosition( id, locked->GetWorldPosition());
			++it;
		}
		else 
		{
			it = m_assigned_ids_.erase( it );
		}
	}
}

void Engine::Resources::Sound::PostUpdate(const float dt)
{
}

void Engine::Resources::Sound::FixedUpdate(const float dt)
{
}

void Engine::Resources::Sound::OnSerialized()
{
}

void Engine::Resources::Sound::Play(const Weak<Components::Transform>& transform, const Vector3& velocity, bool loop)
{
	if ( !m_assigned_ids_.contains( transform ) )
	{
		if ( const Strong<Components::Transform>& locked = transform.lock() )
		{
			ISoundAPI &si = s_sa.GetInterface();
			
			if ( SoundChannelID assigned_id = 0; 
				si.PlaySound( m_primitive_.get(), locked->GetWorldPosition(), velocity, loop, assigned_id ) )
			{
				m_assigned_ids_.emplace( transform, assigned_id );
			}
		}
	}
}

bool Engine::Resources::Sound::IsPlaying(const Weak<Components::Transform>& transform)
{
	return !m_assigned_ids_.empty();
}

void Engine::Resources::Sound::Stop(const Weak<Components::Transform>& transform)
{
	if ( !m_assigned_ids_.contains( transform ) )
	{
		if ( const Strong<Components::Transform>& locked = transform.lock() )
		{
			ISoundAPI &si = s_sa.GetInterface();
			
			if ( si.StopSound( m_assigned_ids_.at(transform) ) )
			{
				m_assigned_ids_.erase( locked );
			}
		}
	}
}

void Engine::Resources::Sound::StopLoop(const Weak<Components::Transform>& transform)
{
	if ( !m_assigned_ids_.contains( transform ) )
	{
		if (const Strong<Components::Transform>& locked = transform.lock())
		{
			ISoundAPI &si = s_sa.GetInterface();
			si.StopLoop( m_primitive_.get(), m_assigned_ids_.at( transform ) );
		}
	}
}

void Engine::Resources::Sound::UpdatePositionAndVelocity(const Weak<Components::Transform>& transform, const Vector3& velocity)
{
	if (!m_assigned_ids_.contains(transform))
	{
		if (const Strong<Components::Transform>& locked = transform.lock())
		{
			ISoundAPI &si = s_sa.GetInterface();
			si.UpdatePosition( m_assigned_ids_.at(transform), locked->GetWorldPosition(), velocity );
		}
	}
}

void Engine::Resources::Sound::SetRollOff(const UINT roll_off) const
{
}

void Engine::Resources::Sound::SetMinDistance(const float min_distance)
{
	m_min_distance_ = min_distance;
	if (m_primitive_)
	{
		m_primitive_->SetMinDistance(min_distance);
	}
}

void Engine::Resources::Sound::SetMaxDistance(const float max_distance)
{
	m_max_distance_ = max_distance;
	if (m_primitive_)
	{
		m_primitive_->SetMaxDistance(max_distance);
	}
}

void Engine::Resources::Sound::Load_INTERNAL()
{
	ISoundAPI &si = s_sa.GetInterface();
	m_primitive_ = Unique<decltype(m_primitive_)::element_type, SoundDeleter>( si.NewSound( GetPath() ) );

	if (m_primitive_)
	{
		m_primitive_->SetMinDistance( m_min_distance_ );
		m_primitive_->SetMaxDistance( m_max_distance_ );
	}
}

void Engine::Resources::Sound::Unload_INTERNAL()
{
	m_primitive_.reset();
}

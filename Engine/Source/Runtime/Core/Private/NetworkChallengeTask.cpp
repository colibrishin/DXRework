#include "NetworkChallengeTask.h"
#include "INetworkAPI.h"
#include "NetworkMessageTask.h"

#include <execution>
#include <iostream>

Engine::NetworkChallengeTask::NetworkChallengeTask()
{
    g_network_accessor.GetMessageTask().onHostAdded.Listen( get_bound_challenge() );
    g_network_accessor.GetMessageTask().onHostRemoved.Listen( get_bound_remove() );
   
    m_challenge_task_running_ = true;
    m_challenge_task_ = std::async( std::launch::async, &NetworkChallengeTask::KeepChallenge, this );
}

bool Engine::NetworkChallengeTask::Convert( const RawNetMessage& message, Unique<NetMessage>& out_message )
{
    return NetworkTaskTypeProxy<ChallengeMessage>::Convert( message, out_message );
}

bool Engine::NetworkChallengeTask::Validate( const RawNetMessage& message ) const
{
    return NetworkTaskTypeProxy<ChallengeMessage>::Validate( message );
}

void Engine::NetworkChallengeTask::Cleanup()
{
    g_network_accessor.GetMessageTask().onHostAdded.Remove( get_bound_challenge() );
    g_network_accessor.GetMessageTask().onHostRemoved.Remove( get_bound_remove() );

    {
        std::lock_guard l( m_wait_mtx_ );
        m_challenge_task_running_ = false;
    }
    m_wait_cv_.notify_all();
    m_challenge_task_.wait();

    m_challenge_status_.clear();
    m_last_challenge_.clear();
}

void Engine::NetworkChallengeTask::Challenge( const NetID new_host )
{
    INetworkAPI& api = g_network_accessor.GetInterface();
    NetMessageDescription desc;
    desc.dst = new_host;
    desc.targetTask = this;
    api.Send( UDP, std::move( desc ), ChallengeMessage{} );

    std::lock_guard l( m_mtx_ );
    m_challenge_status_[ new_host ] = false;
}

void Engine::NetworkChallengeTask::Consume( NetMessageDescription&& desc, Unique<NetMessage>&& message )
{
    const NetMessageDescription moved_desc = std::move( desc );
    Unique<NetMessage>    msg  = std::move( message );
    CONSOLE_OUT( GetTypeName(), "Alive Challenge received from {}", moved_desc.src );

    if ( std::lock_guard l( m_mtx_ ); m_challenge_status_.contains( moved_desc.src ) )
    {
        if ( !m_last_challenge_.contains( moved_desc.src ) || NeedAck( moved_desc.src ) )
        {
            CONSOLE_OUT( GetTypeName(), "Received challenge, challenging back" )
            Challenge( moved_desc.src );
        }

        CONSOLE_OUT( GetTypeName(), "Last challenge time updated for {}", moved_desc.src );
        m_last_challenge_[ moved_desc.src ] = std::chrono::high_resolution_clock::now();
        m_challenge_status_[ moved_desc.src ] = true;
    }
}

void Engine::NetworkChallengeTask::Remove( const NetID removal )
{
    std::lock_guard l( m_mtx_ );
    if ( m_challenge_status_.contains( removal ) )
    {
        m_challenge_status_.erase( removal ); 
    }
    if ( m_last_challenge_.contains( removal ) )
    {
        m_last_challenge_.erase( removal );
    }
}

void Engine::NetworkChallengeTask::KeepChallenge()
{
    // Short rounds: while a host owes an answer (NeedAck, before IsWaitDone), each round re-sends the challenge,
    // so a lost UDP packet is retried within the 30 s wait window.
    constexpr static std::chrono::seconds sleeping( 5 );

    while ( m_challenge_task_running_ )
    {
        // Hosts are added and removed from other threads (Consume, onHostAdded/Removed): work on a snapshot.
        std::vector<NetID> hosts;
        {
            std::lock_guard l( m_mtx_ );
            hosts.reserve( m_challenge_status_.size() );
            for ( const auto& [ id, status ] : m_challenge_status_ )
            {
                hosts.push_back( id );
            }
        }

        for ( const NetID id : hosts )
        {
            if ( NeedAck( id ) && !HaveAck( id ) && IsWaitDone( id ) )
            {
                CONSOLE_OUT( GetTypeName(), "Liveness check failed for {}, Removing from host...", id );
                Remove( id );
                continue;
            }

            if ( NeedAck( id ) && HaveAck( id ) )
            {
                CONSOLE_OUT( GetTypeName(), "Liveness check success with {}", id );
                std::lock_guard l( m_mtx_ );
                m_challenge_status_[ id ] = true;
            }

            if ( NeedAck( id ) )
            {
                CONSOLE_OUT( GetTypeName(), "Liveness check time out for {}, Send the challenge to the host...", id );
                Challenge( id );
            }
        }

        // Cleanup() wakes this up to stop.
        std::unique_lock l( m_wait_mtx_ );
        m_wait_cv_.wait_for( l, sleeping, [ this ] { return !m_challenge_task_running_; } );
    }
}

bool Engine::NetworkChallengeTask::HaveAck( const NetID id ) const
{
    if ( std::lock_guard l( m_mtx_ ); m_challenge_status_.contains( id ) )
    {
        return m_challenge_status_.at( id );
    }

    return false;
}

bool Engine::NetworkChallengeTask::NeedAck( const NetID id ) const
{
    constexpr static std::chrono::minutes interval( 1 );
    const std::chrono::steady_clock::time_point& current_time = std::chrono::high_resolution_clock::now();

    if (std::lock_guard l(m_mtx_); m_last_challenge_.contains(id))
    {
        return interval < current_time - m_last_challenge_.at( id );
    }
      
    return false;
}

bool Engine::NetworkChallengeTask::IsWaitDone( const NetID id ) const
{
    // handshake start time 1 minutes + handshake wait time 30 seconds
    constexpr static std::chrono::duration<long long> interval = std::chrono::minutes(1) + std::chrono::seconds(30);
    const std::chrono::steady_clock::time_point&      current_time = std::chrono::high_resolution_clock::now();

    if ( std::lock_guard l(m_mtx_); m_last_challenge_.contains( id ) )
    {
        return interval < current_time - m_last_challenge_.at( id );
    }

    return false;
}

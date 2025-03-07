#include "NetworkChallengeTask.h"
#include "NetworkChallengeTask.generated.h"
#include "INetworkAPI.h"
#include "NetworkMessageTask.h"

#include <execution>

Engine::NetworkChallengeTask::NetworkChallengeTask()
{
    const auto& challenge_func = std::bind_front( &NetworkChallengeTask::Challenge, this );
    const auto& removal_func   = std::bind_front( &NetworkChallengeTask::Remove, this );
    g_network_accessor.GetMessageTask().onHostAdded.Listen( challenge_func );
    g_network_accessor.GetMessageTask().onHostRemoved.Listen( removal_func );
   
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
    const auto& challenge_func = std::bind_front( &NetworkChallengeTask::Challenge, this );
    const auto& removal_func   = std::bind_front( &NetworkChallengeTask::Remove, this );
    g_network_accessor.GetMessageTask().onHostAdded.Remove( challenge_func );
    g_network_accessor.GetMessageTask().onHostRemoved.Remove( removal_func );

    m_challenge_task_running_ = false;
    m_challenge_sleeper_.notify_all();
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
    NetMessageDescription moved_desc = std::move( desc );
    Unique<NetMessage>    msg  = std::move( message );

    if ( std::lock_guard l( m_mtx_ ); m_challenge_status_.contains( moved_desc.src ) )
    {
        // First challenged host received the ack.
        m_challenge_status_[ moved_desc.src ] = true;

        // if last challenge does not exists, then it would be the handshake.
        if ( !m_last_challenge_.contains( moved_desc.src ) )
        {
            Challenge( moved_desc.src );
            m_challenge_status_[ moved_desc.src ] = true;
        }

        m_last_challenge_[ moved_desc.src ] = std::chrono::high_resolution_clock::now();
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
    constexpr static std::chrono::seconds sleeping( 30 );

    while ( m_challenge_task_running_ )
    {
        const std::chrono::steady_clock::time_point& checktime = std::chrono::high_resolution_clock::now();

        for (auto it = m_challenge_status_.begin(); it != m_challenge_status_.end(); )
        {
            const decltype( m_challenge_status_ )::value_type& pair = *it;

            if ( !HaveAck( pair.first ) )
            {
                auto next_it = std::next( it );
                Remove( pair.first );
                it = next_it;
                continue;
            }

            if ( NeedAck( pair.first ) )
            {
                Challenge( pair.first );
                ++it;
            }
        }

        std::unique_lock l( m_sleeper_mtx_ );
        m_challenge_sleeper_.wait_for( l, sleeping );
    }
}

bool Engine::NetworkChallengeTask::HaveAck( const NetID id )
{
    if (std::lock_guard l(m_mtx_); m_challenge_status_.contains(id))
    {
        return m_challenge_status_.at( id ) && !NeedAck( id );
    }

    return false;
}

bool Engine::NetworkChallengeTask::NeedAck( const NetID id )
{
    constexpr static std::chrono::minutes interval( 1 );
    const std::chrono::steady_clock::time_point& checktime = std::chrono::high_resolution_clock::now();

    if (std::lock_guard l(m_mtx_); m_last_challenge_.contains(id))
    {
        return interval < checktime - m_last_challenge_.at( id );
    }
      
    return false;
}

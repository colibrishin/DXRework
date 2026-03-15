#include "NetworkMessageTask.h"

#include "INetworkAPI.h"
#include "NetworkTask.h"
#include "NetworkChallengeTask.h"

inline void Engine::NetworkMessageTask::Initialize()
{
    GetConsumers().AddConsumer( new NetworkChallengeTask() );
}

inline void Engine::NetworkMessageTask::Shutdown()
{
    // Stop gracefully.
    Poll();
    Cleanup();
}

inline void Engine::NetworkMessageTask::Poll()
{
    {
        std::lock_guard l(m_consume_mutex);
        if (m_consume_ready_queue_.empty())
        {
            if (!m_buffer_consume_ready_queue_.empty())
            {
                std::swap(m_buffer_consume_ready_queue_, m_consume_ready_queue_);   
            }

            return;
        }
    }
    
    m_consume_running_ = true;
    
    while ( m_consume_running_ && !m_consume_ready_queue_.empty() )
    {
        ConsumeMessage( m_consume_ready_queue_.front() );
        m_consume_ready_queue_.pop();
    }

    m_consume_running_ = false;
}

Engine::NetID Engine::NetworkMessageTask::AddNewHost( NetHost&& other )
{
    const NetID id = m_net_hosts_.emplace( std::move( other ) );
    onHostAdded.Broadcast( id );
    return id;
}

void Engine::NetworkMessageTask::RemoveHost( const NetID id )
{ 
    if ( m_net_hosts_.remove( id ) )
    {
        onHostRemoved.Broadcast( id );
    }
}

const Engine::NetHost* Engine::NetworkMessageTask::GetNetHost( const NetID id ) const
{
    return m_net_hosts_.find( id );
}

const Engine::NetHost* Engine::NetworkMessageTask::GetNetHost( const std::array<uint8_t, 4>& address, const uint16_t port, const eNetSendType type ) const
{
    return m_net_hosts_.find( address, port, type );
}

void Engine::NetworkMessageTask::PushConsumeReady( const NetMessageDescription& desc, Unique<NetMessage>&& msg )
{
    std::lock_guard l( m_consume_mutex );
    m_consume_ready_queue_.emplace( desc, std::move( msg ) );
}

inline Engine::NetworkConsumers& Engine::NetworkMessageTask::GetConsumers()
{
    return m_consumers_;
}

inline void Engine::NetworkMessageTask::Cleanup()
{ }

inline void Engine::NetworkMessageTask::ConsumeMessage( std::pair<NetMessageDescription, Unique<NetMessage>>& pair ) const
{
    if ( INetworkTask* target_task = pair.first.targetTask )
    {
        target_task->Consume( std::move( pair.first ), std::move( pair.second ) );
    }
}

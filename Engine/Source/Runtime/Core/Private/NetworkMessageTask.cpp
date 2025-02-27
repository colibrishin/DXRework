#include "NetworkMessageTask.h"
#include "NetworkMessageTask.generated.h"

inline void Engine::NetworkMessageTask::Initialize()
{
    m_rx_future_      = std::async( std::launch::async, &NetworkMessageTask::Receive, this );
    m_tx_future_      = std::async( std::launch::async, &NetworkMessageTask::Send, this );
    m_consume_future_ = std::async( std::launch::deferred, &NetworkMessageTask::Consume, this );
}

inline void Engine::NetworkMessageTask::Shutdown()
{
    m_rx_running_      = false;
    m_tx_running_      = false;
    m_consume_running_ = false;

    // Stop gracefully.
    m_rx_future_.wait();
    m_tx_future_.wait();
    m_consume_future_.wait();

    Cleanup();
}

inline void Engine::NetworkMessageTask::Pool() const
{
    m_consume_future_.wait();
}

inline bool Engine::NetworkMessageTask::IsRunning() const
{
    return m_rx_running_ || m_tx_running_;
}

inline void Engine::NetworkMessageTask::PushMessage( Unique<NetMessage>&& message )
{
    if ( m_tx_running_ )
    {
        std::lock_guard l( m_tx_mutex_ );
        m_buffer_tx_queue_.emplace( std::move( message ) );
    }
}

inline Engine::NetworkConsumers& Engine::NetworkMessageTask::GetConsumers()
{
    return m_consumers_;
}

inline void Engine::NetworkMessageTask::Consume()
{
    m_consume_running_ = true;
    ConsumeImpl();
    m_consume_running_ = false;
}

inline void Engine::NetworkMessageTask::ConsumeImpl()
{
    while ( m_consume_running_ && !m_consume_ready_queue_.empty() )
    {
        Unique<NetMessage>&& msg = std::move( m_consume_ready_queue_.front() );
        m_consume_ready_queue_.pop();
        ConsumeMessage( std::move( msg ) );
    }
}

inline void Engine::NetworkMessageTask::Send()
{
    m_tx_running_ = true;
    while ( m_tx_running_ )
    {
        SendImpl();
    }
}

inline void Engine::NetworkMessageTask::SendImpl()
{
    {
        std::lock_guard l( m_tx_mutex_ );
        if ( !m_buffer_tx_queue_.empty() && m_tx_queue_.empty() )
        {
            std::swap( m_tx_queue_, m_buffer_tx_queue_ );
        }
    }

    while ( !m_tx_queue_.empty() )
    {
        Unique<NetMessage>&& msg = std::move( m_tx_queue_.front() );
        SendMessage( std::move( msg ) );
        m_tx_queue_.pop();
    }
}

inline void Engine::NetworkMessageTask::Receive()
{
    m_rx_running_ = true;
    while ( m_rx_running_ )
    {
        ReceiveImpl();
    }
}

inline void Engine::NetworkMessageTask::ReceiveImpl()
{
    {
        std::lock_guard l( m_rx_mutex_ );
        if ( !m_buffer_rx_queue_.empty() && m_rx_queue_.empty() )
        {
            std::swap( m_rx_queue_, m_buffer_rx_queue_ );
        }
    }

    while ( !m_rx_queue_.empty() )
    {
        const RawNetMessage& msg = m_rx_queue_.front();
        ReceiveMessage( msg );
        m_tx_queue_.pop();
    }
}

inline void Engine::NetworkMessageTask::Cleanup()
{ }

inline void Engine::NetworkMessageTask::SendMessage( Unique<NetMessage>&& message ) const
{
    INetworkAPI& ni = s_nia.GetInterface();

    if ( const INetworkTask* task = m_consumers_.GetConsumer( message->targetTask ) )
    {
        RawNetMessage msg;
        task->Convert( message, msg );
        ni.Send( msg );
    }
}

inline void Engine::NetworkMessageTask::ReceiveMessage( const RawNetMessage& message )
{
    if ( const INetworkTask* target_task = m_consumers_.GetConsumer( message ) )
    {
        if ( Unique<NetMessage> msg; target_task->Convert( message, msg ) )
        {
            std::lock_guard l( m_consume_ready_mutex_ );
            m_consume_ready_queue_.emplace( std::move( msg ) );
        }
    }
}

inline void Engine::NetworkMessageTask::ConsumeMessage( Unique<NetMessage>&& message ) const
{
    if ( INetworkTask* target_task = m_consumers_.GetConsumer( message->targetTask ) )
    {
        target_task->Consume( std::move( message ) );
    }
}

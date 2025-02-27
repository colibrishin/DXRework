#include "NetworkConsumers.h"
#include "NetworkConsumers.generated.h"
#include "INetworkAPI.h"

Engine::INetworkTask* Engine::NetworkConsumers::GetConsumer( const RawNetMessage& message ) const
{
    std::lock_guard l( m_mutex_ );
    const NetMessage&  msg = reinterpret_cast<const NetMessage&>( *message.rawData.data() );

    if ( m_network_consumers_.contains( msg.targetTask ) )
    {
        return m_network_consumers_.at( msg.targetTask ).get();
    }

    return nullptr;
}

Engine::INetworkTask* Engine::NetworkConsumers::GetConsumer( HashType hash ) const
{
    std::lock_guard l( m_mutex_ );
    if ( m_network_consumers_.contains( hash ) )
    {
        return m_network_consumers_.at( hash ).get();
    }

    return nullptr;
}

void Engine::NetworkConsumers::AddConsumer( INetworkTask* task )
{
    std::lock_guard l( m_mutex_ );
    if ( !m_network_consumers_.contains( task->GetTypeHash() ) )
    {
        m_network_consumers_.emplace( task->GetTypeHash(), task );
    }
}

void Engine::NetworkConsumers::Cleanup()
{
    std::lock_guard l( m_mutex_ );
    for ( const Unique<INetworkTask>& task : m_network_consumers_ | std::views::values )
    {
        task->Cleanup();
    }
}

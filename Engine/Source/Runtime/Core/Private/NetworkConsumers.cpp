#include "NetworkConsumers.h"
#include "INetworkAPI.h"

Engine::NetworkConsumers::NetworkConsumers()
{
}

Engine::NetworkConsumers::~NetworkConsumers()
{
    Cleanup();
}

Engine::INetworkTask* Engine::NetworkConsumers::GetConsumer( const NetMessageHeaderType& header ) const
{
    std::lock_guard l( m_mutex_ );
    if ( const auto& it = std::ranges::find_if( m_network_consumers_,
                                                [ &header ]( const auto& task )
                                                {
                                                    return header.targetTask == task.first->v;
                                                } );
        it != m_network_consumers_.end() )
    {
        return it->second.get();
    }

    return nullptr;
}

Engine::INetworkTask* Engine::NetworkConsumers::GetConsumer( const HashType hash ) const
{
    if ( std::lock_guard l( m_mutex_ ); m_network_consumers_.contains( hash ) )
    {
        return m_network_consumers_.at( hash ).get();
    }

    return nullptr;
}

void Engine::NetworkConsumers::AddConsumer( INetworkTask* task )
{
    if ( std::lock_guard l( m_mutex_ ); !m_network_consumers_.contains( task->GetTypeHash() ) )
    {
        m_network_consumers_.emplace( task->GetTypeHash(), task );
    }
}

void Engine::NetworkConsumers::Cleanup()
{
    // Cleanup waits for the tasks' listeners and threads, which look consumers up through GetConsumer (this mutex):
    // call it without holding the lock.
    std::vector<INetworkTask*> tasks;
    {
        std::lock_guard l( m_mutex_ );
        for ( const Unique<INetworkTask>& task : m_network_consumers_ | std::views::values )
        {
            tasks.push_back( task.get() );
        }
    }

    for ( INetworkTask* task : tasks )
    {
        task->Cleanup();
    }
}

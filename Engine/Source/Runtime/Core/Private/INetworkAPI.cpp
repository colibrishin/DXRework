#include "INetworkAPI.h"
#include "INetworkAPI.generated.h"
#include "NetworkMessageTask.h"
#include "NetworkTask.h"
#include "NetworkChallengeTask.h"

ENGINE_CORE_API Engine::INetworkAPIAccessor Engine::g_network_accessor = {};

void Engine::INetworkAPI::Initialize()
{
    if ( !Open(UDP) ||
#if CLIENT || WITH_EDITOR
         !Bind(UDP, 51211) )
#else
         !Bind(UDP, 60901) )
#endif
    {
        assert( nullptr );
    }

    Listen( UDP );
}

void Engine::INetworkAPI::HandleReceived( const eNetSendType            type,
                                          const std::array<uint8_t, 4>& remote_address,
                                          const uint16_t                remote_port,
                                          const RawNetMessage&          msg )
{
    auto           header = reinterpret_cast<const NetMessageHeaderType&>( *msg.data() );
    INetworkTask*  task   = g_network_accessor.GetMessageTask().GetConsumers().GetConsumer( header );
    const NetHost* host   = g_network_accessor.GetMessageTask().GetNetHost( remote_address, remote_port, type );

    if ( !task )
    {
        // Invalid
        return;
    }

    if ( !task->Validate( msg ) )
    {
        // Invalid
        return;
    }

    if ( !host )
    {
        if ( task->GetTypeHash() == NetworkChallengeTask::StaticTypeHash() )
        {
            const NetID id = g_network_accessor.GetMessageTask().AddNewHost( NetHost( remote_address, type, remote_port ) );
            host = g_network_accessor.GetMessageTask().GetNetHost( id );
        }
        else
        {
            // messsage from unknown host.
            return;
        }
    }

    NetMessageDescription desc;
    desc.src        = host->id();
    desc.targetTask = task;

    Unique<NetMessage> message_body;
    task->Convert( msg, message_body );
    g_network_accessor.GetMessageTask().PushConsumeReady( std::move( desc ), std::move( message_body ) );
}

Engine::INetworkTask* Engine::INetworkAPI::ResolveTask( const NetMessageHeaderType& header ) const
{
    NetworkMessageTask& task     = g_network_accessor.GetMessageTask();
    INetworkTask*       msg_task = task.GetConsumers().GetConsumer( header );
    return msg_task;
}

Engine::INetworkTask* Engine::INetworkAPI::ResolveTask( const HashType type ) const
{
    NetworkMessageTask& task     = g_network_accessor.GetMessageTask();
    INetworkTask*       msg_task = task.GetConsumers().GetConsumer( type );
    return msg_task;
}

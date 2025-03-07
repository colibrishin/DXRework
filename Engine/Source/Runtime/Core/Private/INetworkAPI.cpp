#include "INetworkAPI.h"
#include "INetworkAPI.generated.h"
#include "NetworkMessageTask.h"
#include "NetworkTask.h"

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

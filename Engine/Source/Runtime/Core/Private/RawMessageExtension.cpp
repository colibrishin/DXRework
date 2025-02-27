#include "RawMessageExtension.h"
#include "NetworkTask.h"
#include "INetworkAPI.h"

Engine::Unique<Engine::NetMessage>&& Engine::RawMessageExtension::GetMessage( const RawNetMessage& msg ) const
{
    const NetMessage& casting = reinterpret_cast<const NetMessage&>( *msg.rawData.data() );

    if ( INetworkTask* target_task = s_nia.GetInterface().GetMessageTask().GetConsumers().GetConsumer( casting.targetTask ) )
    {
        Unique<NetMessage> ret_msg;
        if ( target_task->Convert( msg, ret_msg ) )
        {
            return std::move( ret_msg );
        }
    }

    return nullptr;
}

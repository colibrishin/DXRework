#include "BoostSocketWrapper.h"
#include "BoostSocketWrapper.generated.h"

#include "NetworkMessageTask.h"
#include "NetworkTask.h"
#include "NetworkChallengeTask.h"

void Engine::BoostSocketWrapper::Initialize()
{
    INetworkAPI::Initialize();
}

void Engine::BoostSocketWrapper::Shutdown()
{
    m_udp_router_.Destory();
}

bool Engine::BoostSocketWrapper::Open( const eNetSendType type )
{
    switch ( type )
    {
        case UDP:
            return m_udp_router_.open();
        default:
            break;
    }

    return false;
}

bool Engine::BoostSocketWrapper::Bind( const eNetSendType type, const uint16_t port )
{
    switch ( type )
    {
        case UDP:
            return m_udp_router_.bind( port );
        default:
            break;
    }

    return false;
}

bool Engine::BoostSocketWrapper::Listen( const eNetSendType type, const NetHost* const host )
{
    if ( const NetHost* host = g_network_accessor.GetMessageTask().GetNetHost( host->ip ) )
    {
        switch ( type )
        {
            default:
                break;
        }
    }

    return false;
}

bool Engine::BoostSocketWrapper::Listen( const eNetSendType type )
{
    switch ( type )
    {
        case UDP:
            m_udp_router_.receive_from(
                    [ & ]( const boost::asio::ip::udp::endpoint& endpoint,
                           const boost::asio::mutable_buffer&    buffer,
                           const boost::system::error_code&      ec,
                           size_t                                read )
                    {
                        if ( ec )
                        {
                            OutputDebugStringA( ec.message().c_str() );
                            return;
                        }

                        if ( !read )
                        {
                            // Invalid
                            return;
                        }

                        if ( read > 0 )
                        {
                            // Copy the data
                            RawNetMessage message{ buffer.data(), read };
                            auto          header = reinterpret_cast<const NetMessageHeaderType&>( *message.data() );
                            INetworkTask* task =
                                    g_network_accessor.GetMessageTask().GetConsumers().GetConsumer( header );

                            if ( !task )
                            {
                                // Invalid
                                return;
                            }

                            if ( !task->Validate( message ) )
                            {
                                // Invalid
                                return;
                            }

                            const NetHost* host = ResolveHost( endpoint );

                            if ( !host )
                            {
                                if ( task->GetTypeHash() == NetworkChallengeTask::StaticTypeHash() )
                                {
                                    g_network_accessor.GetMessageTask().AddNewHost(
                                            { .ip  = endpoint.address().to_v4().to_bytes(),
                                              .tcp = ( uint16_t )-1,
                                              .udp = endpoint.port() } );

                                    host = ResolveHost( endpoint );
                                }
                                else
                                {
                                    // messsage from unknown host.
                                    return;
                                }
                            }

                            NetMessageDescription desc;
                            desc.src        = host->id;
                            desc.targetTask = task;

                            Unique<NetMessage> message_body;
                            task->Convert( message, message_body );
                            g_network_accessor.GetMessageTask().PushConsumeReady( std::move( desc ),
                                                                                  std::move( message_body ) );
                        }
                    } );
            return true;
        default:
            break;
    }

    return false;
}

bool Engine::BoostSocketWrapper::Connect( const std::string_view ip, const unsigned short port )
{
    return false;
}

void Engine::BoostSocketWrapper::sendImpl( const eNetSendType      type,
                                           NetMessageDescription&& desc,
                                           RawNetMessage&&         message )
{
    const NetMessageDescription moved_desc = std::move( desc );
    const NetworkMessageTask&   task       = g_network_accessor.GetMessageTask();
    const NetHost* const        host       = task.GetNetHost( moved_desc.dst );

    if ( !host )
    {
        return;
    }

    if ( type == UDP )
    {
        boost::asio::ip::basic_endpoint<enum_to_protocol<UDP>::type> endpoint( boost::asio::ip::make_address_v4( host->ip ),
                                                                               host->GetPort<UDP>() );
#if WITH_DEBUG
        // host - network byte order check
        assert( endpoint.port() == host->GetPort<UDP>() );
#endif

        Send<UDP>( std::move( endpoint ), std::move( message ) );
    }
    else if ( type == TCP )
    {
        boost::asio::ip::basic_endpoint<enum_to_protocol<TCP>::type> endpoint( boost::asio::ip::make_address_v4( host->ip ),
                                                                               host->GetPort<TCP>() );
#if WITH_DEBUG
        // host - network byte order check
        assert( endpoint.port() == host->GetPort<UDP>() );
#endif
        
        Send<TCP>( std::move( endpoint ), std::move( message ) );
    }
}

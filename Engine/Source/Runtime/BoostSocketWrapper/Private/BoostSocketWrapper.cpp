#include "BoostSocketWrapper.h"
#include "BoostSocketWrapper.generated.h"

#include "NetworkMessageTask.h"
#include "NetworkTask.h"

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
    switch (type)
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
                        // no receive from endpoint?
                    }

                    if ( !read )
                    {
                        // Invalid
                    }

                    if ( read > 0 )
                    {
                        // Copy the data
                        RawNetMessage message{ buffer.data(), read };
                        auto          header = reinterpret_cast<const NetMessageHeaderType&>( *message.data() );
                        INetworkTask* task   = g_network_accessor.GetMessageTask().GetConsumers().GetConsumer( header );

                        if ( !task )
                        {
                            // Invalid
                        }

                        if ( !task->Validate( message ) )
                        {
                            // invalid
                        }

                        const NetHost* host = ResolveHost( endpoint );

                        if ( !host )
                        {
                            // invalid
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

    std::array<unsigned char, 4> cast;
    std::copy_n( host->ip.begin(), cast.size(), cast.begin() );

    if ( type == UDP )
    {
        boost::asio::ip::basic_endpoint<enum_to_protocol<UDP>::type> endpoint{ boost::asio::ip::make_address_v4( cast ),
                                                                               host->GetPort<UDP>() };
        Send<UDP>( std::move( endpoint ), std::move( message ) );
    }
    else if ( type == TCP )
    {
        boost::asio::ip::basic_endpoint<enum_to_protocol<TCP>::type> endpoint{ boost::asio::ip::make_address_v4( cast ),
                                                                               host->GetPort<TCP>() };
        Send<TCP>( std::move( endpoint ), std::move( message ) );
    }
}



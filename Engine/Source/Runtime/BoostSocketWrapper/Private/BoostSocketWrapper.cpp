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
    m_udp_router_.Destroy();
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

bool Engine::BoostSocketWrapper::Listen( const eNetSendType type, const NetHost* const h )
{
    if ( const NetHost* host = g_network_accessor.GetMessageTask().GetNetHost( h->address(), h->port(), h->type() ) )
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
            m_udp_router_.receive_from( std::bind( &BoostSocketWrapper::ReceiveHandler,
                                                   this,
                                                   type,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3,
                                                   std::placeholders::_4 ) );
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

void Engine::BoostSocketWrapper::ReceiveHandler( const eNetSendType                    type,
                                                 const boost::asio::ip::udp::endpoint& endpoint,
                                                 const boost::asio::mutable_buffer&    buffer,
                                                 const boost::system::error_code&      ec,
                                                 size_t                                read )
{
    if ( ec )
    {
        CONSOLE_OUT( GetTypeName(), "Error {} : {}", ec.value(), ec.message().c_str() )
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
        RawNetMessage msg( buffer.data(), read );
        HandleReceived( type, endpoint.address().to_v4().to_bytes(), endpoint.port(), msg );
    }
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
        boost::asio::ip::basic_endpoint<enum_to_protocol<UDP>::type> endpoint(
                boost::asio::ip::make_address_v4( host->address() ), host->port() );
#if WITH_DEBUG
        // host - network byte order check
        assert( endpoint.port() == host->port() );
#endif

        Send<UDP>( std::move( endpoint ), std::move( message ) );
    }
    else if ( type == TCP )
    {
        boost::asio::ip::basic_endpoint<enum_to_protocol<TCP>::type> endpoint(
                boost::asio::ip::make_address_v4( host->address() ), host->port() );
#if WITH_DEBUG
        // host - network byte order check
        assert( endpoint.port() == host->port() );
#endif
        
        Send<TCP>( std::move( endpoint ), std::move( message ) );
    }
}

#pragma once
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>

#include "INetworkAPI.h"
#include "NetworkMessageTask.h"
#include "BoostSocketWrapper.generated.h"

namespace Engine
{
    template <eNetSendType T>
    struct enum_to_protocol
    { };

    template <>
    struct enum_to_protocol<UDP>
    {
        using type = boost::asio::ip::udp;
    };

    template <>
    struct enum_to_protocol<TCP>
    {
        using type = boost::asio::ip::tcp;
    };

    template <typename T>
    struct protocol_to_enum
    { };

    template <>
    struct protocol_to_enum<boost::asio::ip::udp>
    {
        static constexpr eNetSendType value = UDP;
    };

    template <>
    struct protocol_to_enum<boost::asio::ip::tcp>
    {
        static constexpr eNetSendType value = TCP;
    };

    template <typename T>
    struct max_packet_size
    { };

    template <>
    struct max_packet_size<boost::asio::ip::udp>
    {
        static constexpr size_t value = 65507;
    };

    template <>
    struct max_packet_size<boost::asio::ip::tcp>
    {
        static constexpr size_t value = 65535;
    };

    template <typename T>
    struct local_address_resolver
    {
        std::array<uint8_t, 4> operator()()
        {
            const auto& endpoints = resolver.resolve( boost::asio::ip::host_name(), "" );

            for ( const boost::asio::ip::basic_endpoint<T>& endpoint : endpoints )
            {
                if ( endpoint.address().is_v4() && 
                     !endpoint.address().is_loopback() &&
                     !endpoint.address().is_unspecified() )
                {
                    return endpoint.address().to_v4().to_bytes();
                }
            }

            assert( nullptr );
            return {};
        }

    private:
        inline static boost::asio::io_context context{};
        inline static T::resolver resolver{ context };
    };

    namespace BoostNetwork
    {
        using allocator_type = u_pool_allocator_single<unsigned char>;

        template <eNetSendType Type>
        static allocator_type& getAllocator()
        {
            static allocator_type alloc{};
            return alloc;
        }

        template <eNetSendType Type>
        static unsigned char* allocate( const size_t bytes )
        {
            return getAllocator<Type>().allocate( bytes );
        }

        template <eNetSendType Type>
        static void deallocate( unsigned char* ptr, const size_t bytes )
        {
            getAllocator<Type>().deallocate( ptr, bytes );
        }
    }

    template <typename Protocol>
    struct Context
    {
        using endpoint_type = boost::asio::ip::basic_endpoint<Protocol>;

        ~Context()
        {
            Destory();
        }

        bool open()
        {
            if ( boost::system::error_code ec; m_socket_.open( Protocol::v4(), ec ) )
            {
                OutputDebugStringA( ec.message().c_str() );
                return false;
            }

            return true;
        }

        bool bind( uint16_t port )
        {
            static local_address_resolver<Protocol> resolver{};
            std::array<uint8_t, 4>                  address = resolver();

            while ( true )
            {
                m_local_endpoint_ = { boost::asio::ip::address_v4( address ), port };

                if ( boost::system::error_code ec; m_socket_.bind( m_local_endpoint_, ec ) )
                {
                    ++port;
                }
                else
                {
                    break;
                }
            }

            return true;
        }

        template <typename = std::enable_if_t<std::is_same_v<boost::asio::ip::tcp, Protocol>>>
        bool receive( const std::function<void( const boost::asio::mutable_buffer&, const boost::system::error_code& ec, size_t )>& predicate )
        {
            m_socket_.async_receive(
                    m_recv_buffer_,
                    0,
                    std::bind( predicate, m_recv_buffer_, std::placeholders::_1, std::placeholders::_2 ) );
            return true;
        }

        template <typename = std::enable_if_t<std::is_same_v<boost::asio::ip::udp, Protocol>>>
        bool
        receive_from( const std::function<void( const endpoint_type&, const boost::asio::mutable_buffer&, const boost::system::error_code&, size_t )>& predicate )
        {
            endpoint_type remote_endpoint;
            m_socket_.async_receive_from( m_recv_buffer_,
                                          remote_endpoint,
                                          std::bind( predicate,
                                                     remote_endpoint,
                                                     m_recv_buffer_,
                                                     std::placeholders::_1,
                                                     std::placeholders::_2 ) );
            return true;
        }

        bool send( const boost::asio::mutable_buffer&                                        buffer,
                   const std::function<void( const boost::system::error_code& ec, size_t )>& predicate )
        {
            m_socket_.async_send( buffer, predicate );
            return true;
        }

        template <typename = std::enable_if_t<std::is_same_v<boost::asio::ip::udp, Protocol>>>
        bool send_to( const boost::asio::const_buffer&     buffer,
                      const endpoint_type&                 remote_endpoint,
                      const std::function<void( const endpoint_type&,
                                                const boost::system::error_code&,
                                                size_t )>& predicate )
        {
            m_socket_.async_send_to(
                    buffer,
                    remote_endpoint,
                    std::bind( predicate, remote_endpoint, std::placeholders::_1, std::placeholders::_2 ) );
            return true;
        }

        void Destory()
        {
            m_socket_.cancel();
            m_socket_.close();
            m_context_.stop();
            BoostNetwork::deallocate<protocol_to_enum<Protocol>::value>( ( uint8_t* )m_recv_buffer_.data(),
                                                                         m_recv_buffer_.size() );
        }

        uint16_t GetListenPort() const
        {
            return m_local_endpoint_.port();
        }

    private:
        boost::asio::io_context     m_context_;
        Protocol::socket            m_socket_{ m_context_ };
        endpoint_type               m_local_endpoint_;
        boost::asio::mutable_buffer m_recv_buffer_{ BoostNetwork::allocate<protocol_to_enum<Protocol>::value>(
                                                            max_packet_size<Protocol>::value ),
                                                    max_packet_size<Protocol>::value };
    };

    ECLASS(virtual)
    struct ENGINE_BOOSTSOCKETWRAPPER_API BoostSocketWrapper : public INetworkAPI
    {
        GENERATE_BODY
        void Initialize() override;
        void Shutdown() override;

        bool Open( const eNetSendType type ) override;
        bool Bind( const eNetSendType type, const uint16_t port ) override;

        bool Listen( const eNetSendType type, const NetHost* const host ) override;
        bool Listen( const eNetSendType type ) override;
        bool Connect( const std::string_view ip, const unsigned short port ) override;

    private:
        template <typename Protocol>
        const NetHost* ResolveHost(const boost::asio::ip::basic_endpoint<Protocol>& endpoint)
        {
            return g_network_accessor.GetMessageTask().GetNetHost( endpoint.address().to_v4().to_bytes() );
        }

        void sendImpl( const eNetSendType type, NetMessageDescription&& desc, RawNetMessage&& message ) override;
        
        template <eNetSendType Protocol, typename EndpointProtocolType = typename enum_to_protocol<Protocol>::type>
        void Send( boost::asio::ip::basic_endpoint<EndpointProtocolType>&& dst, RawNetMessage&& message )
        {
            using endpoint_type = boost::asio::ip::basic_endpoint<EndpointProtocolType>;

            if constexpr ( Protocol == UDP )
            {
                const auto& [value, _] = m_sending_message_.emplace( std::move ( message ) );
                assert( value->size() < max_packet_size<EndpointProtocolType>::value );
                m_udp_router_.send_to(
                        boost::asio::buffer( value->data(), value->size() ),
                        dst,
                        [ &, value ]( const endpoint_type& endpoint, const boost::system::error_code& ec, size_t sent )
                        {
                            if ( ec )
                            {
                                OutputDebugStringA( ec.message().c_str() );
                            }

                            m_sending_message_.erase( value );
                        } );
            }
        }

        Context<boost::asio::ip::udp> m_udp_router_;
        
        std::unordered_set<RawNetMessage> m_sending_message_;
    };
}
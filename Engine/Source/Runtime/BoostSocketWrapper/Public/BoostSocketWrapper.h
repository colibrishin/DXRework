#pragma once
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <cmath>
#include <iostream>
#include <execution>

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

#if PLATFORM == Windows
    struct winsock_udp_connreset
    {
        unsigned long value = 0;
        int       name()
        {
            return -1744830452; /* SIO_UDP_CONNRESET */
        }
        unsigned long* data()
        {
            return &value;
        }
    };
#endif
    
    template <typename T>
    struct local_address_resolver
    {
        std::array<uint8_t, 4> operator()()
        {
            const auto& endpoints = resolver.resolve( boost::asio::ip::host_name(), "" );

            for ( const boost::asio::ip::basic_endpoint<T>& endpoint : endpoints )
            {
                if ( endpoint.address().is_v4() && !endpoint.address().is_loopback() &&
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
        inline static typename T::resolver    resolver{ context };
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
    } // namespace BoostNetwork

    template <typename Protocol>
    struct Context
    {
        using endpoint_type = boost::asio::ip::basic_endpoint<Protocol>;

        ~Context()
        {
            Destroy();
        }

        Context()
        {
            m_socket_ = decltype( m_socket_ )( m_context_ );

            const auto& thread_count       = std::min<uint32_t>( std::thread::hardware_concurrency(), 4 );
            const auto& bind_error_handler = std::bind( &Context::contextErrorHandler, this, std::placeholders::_1 );

            std::generate_n( std::back_inserter( m_io_threads_ ),
                             thread_count,
                             [ this, &bind_error_handler ]() {
                                 return std::thread( &Context::contextRunner,
                                                     this,
                                                     std::ref( m_context_ ),
                                                     std::ref( bind_error_handler ) );
                             } );

            const size_t receiving_threads = std::min<size_t>( std::thread::hardware_concurrency(), 4 );
            m_recv_buffers_.resize( receiving_threads );
            m_remote_endpoint_storage_.resize( receiving_threads );
            
            for (size_t i = 0; i < receiving_threads; ++i)
            {
                m_recv_running_.emplace_back( false );
            }

            for ( auto& buffer : m_recv_buffers_ )
            {
                buffer = { BoostNetwork::allocate<protocol_to_enum<Protocol>::value>(
                                   max_packet_size<Protocol>::value ),
                           max_packet_size<Protocol>::value };
            }
        }


        bool open()
        {
            CONSOLE_OUT( "BoostContext", "Opening UDP Socket" )
            if ( boost::system::error_code ec; m_socket_.open( Protocol::v4(), ec ) )
            {
                OutputDebugStringA( ec.message().c_str() );
                CONSOLE_OUT( "BoostContext", "Unable to opening the socket: {}", ec.value() );
                return false;
            }

#if PLATFORM == Windows
            static winsock_udp_connreset conn_reset_flag{};
            m_socket_.set_option( boost::asio::socket_base::reuse_address( true ) );
            m_socket_.io_control( conn_reset_flag );
#endif
            m_running_ = true;

            return true;
        }

        bool bind( uint16_t port )
        {
            if ( !m_running_ )
            {
                return false;
            }

            while ( true )
            {
                CONSOLE_OUT( "BoostContext", "Binding UDP Socket to {}", port )
                m_local_endpoint_ = { Protocol::v4(), port };

                if ( boost::system::error_code ec; m_socket_.bind( m_local_endpoint_, ec ) )
                {
                    CONSOLE_OUT( "BoostContext", "Unable to bind the UDP Socket to {}, trying {}", port, port + 1 )
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
        bool
        receive( const std::function<
                 void( const boost::asio::mutable_buffer&, const boost::system::error_code& ec, size_t )>& predicate )
        {
            if ( !m_running_ )
            {
                return false;
            }

            /*
            m_socket_.async_receive(
                    m_recv_buffer_,
                    0,
                    std::bind( predicate, m_recv_buffer_, std::placeholders::_1, std::placeholders::_2 ) );
            */

            return true;
        }

        using ReceiveHandlerSignature = std::function<void(
                const endpoint_type&, const boost::asio::mutable_buffer&, const boost::system::error_code&, size_t )>;

        template <typename = std::enable_if_t<std::is_same_v<boost::asio::ip::udp, Protocol>>>
        bool receive_from( const ReceiveHandlerSignature& predicate )
        {
            if ( !m_running_ )
            {
                return false;
            }

            CONSOLE_OUT( "BoostContext", "Start receiving..." )

            for ( auto it = m_recv_buffers_.begin(); it != m_recv_buffers_.end(); ++it )
            {
                const ptrdiff_t idx = std::distance( m_recv_buffers_.begin(), it );

                if ( !m_recv_running_[ idx ] )
                {
                    m_socket_.async_receive_from( *it,
                                                  m_remote_endpoint_storage_.at( idx ),
                                                  std::bind( &Context::receiveHandler,
                                                             this,
                                                             idx,
                                                             predicate,
                                                             *it,
                                                             std::placeholders::_1,
                                                             std::placeholders::_2 ) );

                    while ( m_recv_running_[ idx ].exchange( true ) )
                    { }

                    CONSOLE_OUT( "BoostContext", "Start the receiving thread {}", idx )
                }
            }

            return true;
        }

        bool send( const boost::asio::mutable_buffer&                                        buffer,
                   const std::function<void( const boost::system::error_code& ec, size_t )>& predicate )
        {
            if ( !m_running_ )
            {
                return false;
            }

            m_socket_.async_send( buffer, predicate );
            return true;
        }

        template <typename = std::enable_if_t<std::is_same_v<boost::asio::ip::udp, Protocol>>>
        bool send_to(
                const boost::asio::const_buffer& buffer,
                const endpoint_type&             remote_endpoint,
                const std::function<void( const endpoint_type&, const boost::system::error_code&, size_t )>& predicate )
        {
            if ( !m_running_ )
            {
                return false;
            }

            m_socket_.async_send_to(
                    buffer,
                    remote_endpoint,
                    std::bind( predicate, remote_endpoint, std::placeholders::_1, std::placeholders::_2 ) );
            return true;
        }

        void Destroy()
        {
            m_running_ = false;

            for ( std::atomic<bool>& flag : m_recv_running_ )
            {
                flag.store( false );
            }

            m_socket_.close();
            m_work_gurad_.reset();
            m_context_.stop();

            std::ranges::for_each( m_io_threads_,
                                   []( std::thread& elem )
                                   {
                                       if ( elem.joinable() )
                                       {
                                           elem.join();
                                       }
                                   } );

            for ( auto& buffer : m_recv_buffers_ | std::views::reverse )
            {
                BoostNetwork::deallocate<protocol_to_enum<Protocol>::value>( ( uint8_t* )buffer.data(),
                                                                             buffer.size() );
            }
        }

        uint16_t GetListenPort() const
        {
            return m_local_endpoint_.m_port_();
        }

    private:
        void receiveHandler( const size_t                       index,
                             const ReceiveHandlerSignature&     predicate,
                             const boost::asio::mutable_buffer& recv_buffer,
                             const boost::system::error_code&   ec,
                             size_t                             read )
        {
            predicate( m_remote_endpoint_storage_.at( index ), recv_buffer, ec, read );
            
            if ( !ec && m_recv_running_[ index ] )
            {
                m_socket_.async_receive_from( recv_buffer,
                                              m_remote_endpoint_storage_.at( index ),
                                              std::bind( &Context::receiveHandler,
                                                         this,
                                                         index,
                                                         predicate,
                                                         recv_buffer,
                                                         std::placeholders::_1,
                                                         std::placeholders::_2 ) );
            }
        }

        void contextRunner( boost::asio::io_context&                            context,
                            const std::function<void( const std::exception& )>& err_handler )
        {
            while ( true )
            {
                try
                {
                    context.run();
                    break;
                }
                catch ( std::exception& e )
                {
                    err_handler( e );
                }
            }
        }

        void contextErrorHandler( const std::exception& e ){
            CONSOLE_OUT( "BoostContext", "contextRunner throws exception with {}", e.what() )
        }

        boost::asio::io_context m_context_;
        boost::asio::executor_work_guard<decltype( m_context_ )::executor_type> m_work_gurad_{
            boost::asio::make_work_guard( m_context_ )
        };
        std::vector<std::thread>  m_io_threads_;
        typename Protocol::socket m_socket_{ m_context_ };
        endpoint_type             m_local_endpoint_;

        std::vector<endpoint_type>               m_remote_endpoint_storage_;
        std::atomic<bool>                        m_running_;
        std::vector<boost::asio::mutable_buffer> m_recv_buffers_;
        std::deque<std::atomic<bool>>            m_recv_running_;
    };

    ECLASS( virtual )
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
        void ReceiveHandler( const eNetSendType type,
                             const boost::asio::ip::udp::endpoint& endpoint,
                             const boost::asio::mutable_buffer&    buffer,
                             const boost::system::error_code&      ec,
                             size_t                                read );

        template <typename Protocol>
        const NetHost* ResolveHost( const boost::asio::ip::basic_endpoint<Protocol>& endpoint )
        {
            return g_network_accessor.GetMessageTask().GetNetHost(
                    endpoint.address().to_v4().to_bytes(), endpoint.port(), protocol_to_enum<Protocol>::value );
        }

        void sendImpl( const eNetSendType type, NetMessageDescription&& desc, RawNetMessage&& message ) override;

        template <eNetSendType Protocol, typename EndpointProtocolType = typename enum_to_protocol<Protocol>::type>
        void Send( boost::asio::ip::basic_endpoint<EndpointProtocolType>&& dst, RawNetMessage&& message )
        {
            using endpoint_type = boost::asio::ip::basic_endpoint<EndpointProtocolType>;

            if constexpr ( Protocol == UDP )
            {
                const auto& [ value, _ ] = m_sending_message_.emplace( std::move( message ) );
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
} // namespace Engine

#pragma once
#include <vector>

#include "Allocator.h"
#include "SIMDExtension.hpp"
#include "CoreType.h"
#include "TypeLibrary.h"

#if SERVER
#define CONSOLE_OUT(PREFIX, FMT, ...) \
    std::cout << std::format("[{} | {}]: ", std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::high_resolution_clock::now().time_since_epoch() ), PREFIX ); \
    std::cout << std::format(FMT, __VA_ARGS__) << '\n';
#else
#define CONSOLE_OUT(PREFIX, FMT, ...)
#endif

namespace Engine
{
    struct INetworkTask;

#pragma pack( push, 1 )
    struct ENGINE_CORE_API NetMessageHeaderType final
    {
        cityhash::cityhash256 targetTask{};
        // todo: previous message
    };

    struct ENGINE_CORE_API NetMessage
    {};

    template <typename T>
        requires std::is_base_of_v<NetMessage, T> && !std::is_same_v<NetMessage, T> &&
                 !std::is_polymorphic_v<T>
    struct NetMessageSendType final
    {
        NetMessageHeaderType header{};
        T                    body;
    };
#pragma pack( pop )

    struct ENGINE_CORE_API NetMessageDescription final
    {
        NetID         src        = -1;
        NetID         dst        = -1;
        INetworkTask* targetTask = nullptr;
    };

    struct ENGINE_CORE_API RawNetMessage
    {
        RawNetMessage() = default;
        RawNetMessage(const void* data, const size_t size);

        template <typename T>
            requires std::is_base_of_v<NetMessage, T> && !std::is_same_v<NetMessage, T> && !std::is_polymorphic_v<T>
        explicit RawNetMessage( const NetMessageSendType<T>& msg )
        {
            m_raw_data_.resize( sizeof( NetMessageSendType<T> ) );
            SIMDExtension::_mm256_memcpy( m_raw_data_.data(), &msg, sizeof( NetMessageSendType<T> ) );
        } // namespace Engine

        const unsigned char* data() const;
        const unsigned char* data_without_header() const;
        size_t size() const;
        size_t size_without_header() const;
        bool operator==(const RawNetMessage& other) const noexcept;

    private:
        friend struct std::hash<RawNetMessage>;
        std::vector<unsigned char, u_pool_allocator_single<unsigned char>> m_raw_data_{};
    };

    enum ENGINE_CORE_API eNetSendType
    {
        UDP,
        TCP,
        ICMP
    };

    struct ENGINE_CORE_API NetHost
    {
    private:
        friend struct NetHosts;

        uint32_t addr_to_int()
        {
            return ( uint32_t )m_addr_[ 0 ] | ( ( uint32_t )m_addr_[ 1 ] << 8 ) | ( ( uint32_t )m_addr_[ 2 ] << 16 ) |
                   ( ( uint32_t )m_addr_[ 3 ] << 24 );
        }

        NetID                  m_id_ = ( NetID )-1;
        std::array<uint8_t, 4> m_addr_{};
        uint32_t               m_ip_int;
        eNetSendType           m_type_ = UDP;
        uint16_t               m_port_ = ( uint16_t )-1;

        void set_id(const NetID& id)
        {
            m_id_ = id;
        }

    public:
        NetHost( std::array<uint8_t, 4>&& ip, eNetSendType&& type, uint16_t&& port )
            : m_id_( -1 ),
              m_addr_( std::move( ip ) ),
              m_type_( std::move( type ) ),
              m_port_( std::move( port ) )
        {
            m_ip_int = addr_to_int();
        }

        NetHost( const std::array<uint8_t, 4>& ip, const eNetSendType type, const uint16_t port )
            : m_id_( -1 ),
              m_addr_( ip ),
              m_type_( type ),
              m_port_( port )
        {
            m_ip_int = addr_to_int();
        }

        bool operator==(const NetHost& other) const
        {
            return m_addr_ == other.m_addr_ && m_port_ == other.m_port_ && m_type_ == other.m_type_;
        }

        [[nodiscard]] eNetSendType type() const
        {
            return m_type_;
        }

        [[nodiscard]] uint16_t port() const
        {
            return m_port_;
        }

        [[nodiscard]] uint32_t address_integer() const
        {
            return m_ip_int;
        }

        [[nodiscard]] NetID id() const
        {
            return m_id_;
        }

        [[nodiscard]] const decltype(m_addr_)& address() const
        {
            return m_addr_;
        }

        [[nodiscard]] bool IsValid() const noexcept
        {
            return m_id_ != -1 || m_port_ != -1;
        }
    };
}

template <>
struct ENGINE_CORE_API std::hash<Engine::NetHost>
{
    size_t operator()( const Engine::NetHost& host ) const noexcept
    {
        static std::hash<decltype( host.port() )> port_hasher{};
        static std::hash<decltype( host.type() )> type_hasher{};

        size_t value = host.address_integer();
        boost::hash_combine( value, port_hasher( host.port() ) );
        boost::hash_combine( value, type_hasher( host.type() ) );
        return value;
    }
};

template <>
struct ENGINE_CORE_API std::hash<Engine::RawNetMessage>
{
    size_t operator()(const Engine::RawNetMessage& message) const noexcept
    {
        static std::hash<unsigned char> hasher{};
        size_t                          value = 0;

        if (message.size())
        {
            value = message.m_raw_data_[ 0 ];
            for (size_t i = 0; i < message.m_raw_data_.size(); ++i)
            {
                value = hasher( value );   
            }
        }
        
        return value;
    }
};
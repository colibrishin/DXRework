#pragma once
#include <vector>

#include "Allocator.h"
#include "SIMDExtension.hpp"
#include "CoreType.h"
#include "TypeLibrary.h"

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
        NetID                  id = ( NetID )-1;
        std::array<uint8_t, 4> ip{};
        uint16_t               tcp{};
        uint16_t               udp{};

        template <eNetSendType Type>
        uint16_t GetPort() const
        {
            if constexpr ( Type == TCP )
            {
                return tcp;
            }
            else if constexpr ( Type == UDP )
            {
                return udp;
            }
        }
    };
}

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
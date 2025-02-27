#pragma once
#include <vector>
#include "Allocator.h"
#include "CoreType.h"
#include "SIMDExtension.hpp"
#include "TypeLibrary.h"

#include "INetworkAPI.generated.h"

namespace Engine 
{
    struct NetworkMessageTask;

#pragma pack( push, 1 )
    struct ENGINE_CORE_API NetMessage
    {
        unsigned char  sender[ 32 ]{};
        unsigned short txport{};
        unsigned char  receiver[ 32 ]{};
        unsigned short rxport{};
        HashType       targetTask{};
    };
#pragma pack( pop )

    struct ENGINE_CORE_API RawNetMessage
    {
        std::vector<unsigned char, u_pool_allocator_single<unsigned char>> rawData{};

        RawNetMessage()
        {
            rawData.emplace_back( '\0' );
        }

        template <typename T>
            requires std::is_base_of_v<NetMessage, T> && !std::is_same_v<NetMessage, T>
        explicit RawNetMessage( const T& msg )
        {
            rawData.resize( sizeof( T ) );
            SIMDExtension::_mm256_memcpy( rawData.data(), &msg, sizeof( T ) );
            rawData.emplace_back( '\0' );
        }
    };

    ECLASS( virtual, abstract )
    struct ENGINE_CORE_API INetworkAPI
    {
        GENERATE_BODY

        virtual ~INetworkAPI() = default;
        virtual void Initialize() = 0;
        virtual void Shutdown()   = 0;

        virtual bool                Listen( const unsigned short port )                             = 0;
        virtual bool                Connect( const std::string_view ip, const unsigned short port ) = 0;
        virtual void                Send( const RawNetMessage& message )                               = 0;
        virtual NetworkMessageTask& GetMessageTask()                                                = 0;
    };

    struct ENGINE_CORE_API INetworkAPIAccessor
    {
        INetworkAPI& GetInterface() const
        {
            return *m_interface_;
        }

        template <typename T>
        void SetInterface()
        {
            if ( !m_interface_ )
            {
                m_interface_ = std::make_unique<T>();
            }
        }

        void Shutdown()
        {
            if ( m_interface_ )
            {
                m_interface_->Shutdown();
                m_interface_.reset();
            }
        }

    private:
        Unique<INetworkAPI> m_interface_ = nullptr;
    };

    static INetworkAPIAccessor s_nia;
}
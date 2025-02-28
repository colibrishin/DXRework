#pragma once
#include <mutex>
#include <condition_variable>
#include <thread>
#include "CoreType.h"
#include "TypeLibrary.h"
#include "NetworkType.h"
#include "SIMDExtension.hpp"

#include "NetworkTask.generated.h"

namespace Engine 
{
    ECLASS( virtual, abstract )
    struct ENGINE_CORE_API INetworkTask
    {
        GENERATE_BODY
        virtual ~INetworkTask()                                                                   = default;
        virtual void Consume( NetMessageDescription&& desc, Unique<NetMessage>&& message )                                      = 0;
        virtual bool Convert( const RawNetMessage& message, Unique<NetMessage>& out_mssgae )      = 0;
        virtual bool Validate( const RawNetMessage& message ) const = 0;
        virtual void Cleanup()                                                                    = 0;
    };

    template <typename T>
        requires std::is_base_of_v<NetMessage, T> && !std::is_same_v<NetMessage, T> &&
                 !std::is_polymorphic_v<T>
    struct NetworkTaskTypeProxy : INetworkTask
    {
        using message_type      = T;
        ~NetworkTaskTypeProxy() override = default;

        [[nodiscard]] static size_t GetMessageSize()
        {
            return sizeof( T );
        }

        [[nodiscard]] bool Validate( const RawNetMessage& message ) const override
        {
            if ( GetMessageSize() != message.size_without_header() )
            {
                // todo: validate the message (e.g., MD5)
                return false;
            }

            return true;
        }

        [[nodiscard]] bool Convert(const RawNetMessage& message, Unique<NetMessage>& out_message) override
        {
            if (Validate(message))
            {
                out_message = Unique<T>( new T( reinterpret_cast<const T&>( *message.data_without_header() ) ) );
                return true;
            }

            return false;
        }
    };
}
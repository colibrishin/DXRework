#pragma once
#include <mutex>
#include <condition_variable>
#include <thread>
#include "CoreType.h"
#include "TypeLibrary.h"
#include "INetworkAPI.h"
#include "SIMDExtension.hpp"

#include "NetworkTask.generated.h"

namespace Engine 
{
    ECLASS( virtual, abstract )
    struct ENGINE_CORE_API INetworkTask
    {
        GENERATE_BODY
        virtual ~INetworkTask()                                                                   = default;
        virtual void Consume( Unique<NetMessage>&& message )                                         = 0;
        virtual bool Convert( const RawNetMessage& message, Unique<NetMessage>& out_message ) const     = 0;
        virtual bool Convert( const Unique<NetMessage>& message, RawNetMessage& out_raw_message ) const = 0;
        virtual void Cleanup()                                                                    = 0;
    };

    template <typename T>
    struct NetworkTaskTypeProxy : INetworkTask
    {
        using message_type      = T;
        ~NetworkTaskTypeProxy() override = default;

        [[nodiscard]] static size_t GetMessageSize()
        {
            return sizeof( T );
        }

        bool Convert( const RawNetMessage& message, Unique<NetMessage>& out_message ) const override
        {
            if ( GetMessageSize() + 1 != message.rawData.size() )
            {
                return false;
            }

            // todo: validate the message (e.g., MD5)
            out_message = Unique<T>( reinterpret_cast<const T&>( *message.rawData.data() ) );
            return true;
        }

        bool Convert( const Unique<NetMessage>& message, RawNetMessage& out_raw_message ) const override
        {
            if ( message->targetTask != message_type::StaticTypeHash() )
            {
                return false;
            }

            // todo: validate the message (e.g., MD5)
            out_raw_message = RawNetMessage( reinterpret_cast<message_type&>( *message ) );
            return true;
        }
    };
}
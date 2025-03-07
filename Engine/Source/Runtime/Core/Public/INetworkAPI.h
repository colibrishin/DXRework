#pragma once
#include "Allocator.h"
#include "CoreType.h"
#include "SIMDExtension.hpp"
#include "TypeLibrary.h"
#include "NetworkType.h"
#include "NetworkMessageTask.h"

#include "INetworkAPI.generated.h"

namespace Engine
{
    ECLASS( virtual, abstract )
    struct ENGINE_CORE_API INetworkAPI
    {
        GENERATE_BODY

        virtual      ~INetworkAPI()    = default;
        virtual void Initialize() = 0;
        virtual void Shutdown()        = 0;

        virtual bool Open( const eNetSendType type )            = 0;
        virtual bool Bind( const eNetSendType type, const uint16_t port ) = 0;

        virtual bool Listen( const eNetSendType type, const NetHost* const host ) = 0;
        virtual bool Listen( const eNetSendType type )       = 0;
        virtual bool Connect( const std::string_view ip, const unsigned short port )                         = 0;
        template <typename T>
            requires std::is_base_of_v<NetMessage, T> && !std::is_same_v<NetMessage, T> &&
                     !std::is_polymorphic_v<T>
        void Send( const eNetSendType type, NetMessageDescription&& send_desc, T&& message )
        {
            NetMessageSendType<T> packed_msg;
            if ( NetMessageDescription desc    = std::move( send_desc ); 
                 ResolveTask( desc.targetTask->GetTypeHash() ) )
            {
                packed_msg.header.targetTask = desc.targetTask->GetTypeHash()->v;
                packed_msg.body = std::forward<T>( message );
                RawNetMessage msg( packed_msg );
                sendImpl( type, std::move( desc ), std::move( msg ) );
            }
        }

    protected:
        INetworkTask* ResolveTask( const NetMessageHeaderType& header ) const;
        INetworkTask* ResolveTask( const HashType type ) const;
        virtual void  sendImpl( const eNetSendType type, NetMessageDescription&& desc, RawNetMessage&& message ) = 0;
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
                m_interface_->Initialize();
                m_message_task_.Initialize();
            }
        }

        NetworkMessageTask& GetMessageTask()
        {
            return m_message_task_;
        }

        void Shutdown()
        {
            if ( m_interface_ )
            {
                m_interface_->Shutdown();
                m_interface_.reset();
            }
        }

        bool IsValid() const
        {
            return m_interface_.get();
        }

    private:
        NetworkMessageTask  m_message_task_;
        Unique<INetworkAPI> m_interface_ = nullptr;
    };

    extern ENGINE_CORE_API INetworkAPIAccessor g_network_accessor;
}

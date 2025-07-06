#pragma once
#undef SendMessage
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include "CoreType.h"
#include "NetworkConsumers.h"
#include "TypeLibrary.h"
#include "Delegation.hpp"
#include "NetHosts.h"

#include "NetworkMessageTask.generated.h"

DEFINE_DELEGATE( OnHostAdded, const Engine::NetID );
DEFINE_DELEGATE( OnHostRemoved, const Engine::NetID );

namespace Engine
{
    ECLASS( virtual )
    struct ENGINE_CORE_API NetworkMessageTask
    {
        GENERATE_BODY

        DelegateOnHostAdded onHostAdded;
        DelegateOnHostRemoved onHostRemoved;
       
        virtual ~NetworkMessageTask() = default;
        void                  Initialize();
        void                  Shutdown();
        void                  Poll();

        NetID          AddNewHost( NetHost&& other );
        void           RemoveHost( const NetID id );
        const NetHost* GetNetHost( const NetID id ) const;
        const NetHost* GetNetHost( const std::array<uint8_t, 4>& address, const uint16_t port, const eNetSendType type ) const;

        void PushConsumeReady( const NetMessageDescription& desc, Unique<NetMessage>&& msg );

        NetworkConsumers& GetConsumers();

    protected:
        void Cleanup();
        void ConsumeMessage(
                std::pair<NetMessageDescription, Unique<NetMessage>>& pair ) const;

    private:
        mutable std::mutex                                               m_consume_mutex;
        std::queue<std::pair<NetMessageDescription, Unique<NetMessage>>> m_buffer_consume_ready_queue_;
        std::queue<std::pair<NetMessageDescription, Unique<NetMessage>>> m_consume_ready_queue_;

        NetworkConsumers                   m_consumers_;
        NetHosts                           m_net_hosts_;


        std::atomic<bool> m_consume_running_;
    };
}

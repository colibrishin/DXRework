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

#include "NetworkMessageTask.generated.h"

DEFINE_DELEGATE( OnHostAdded, const Engine::NetID )
DEFINE_DELEGATE( OnHostRemoved, const Engine::NetID )

namespace Engine
{
    struct NetHosts
    {
        mutable std::recursive_mutex              m_mtx_;
        std::unordered_set<NetHost>               m_hosts_;
        std::unordered_map<NetID, NetHost>        m_id_wises_;

        NetID emplace( NetHost&& h ) 
        {
            NetHost host = std::move( h );
            
            if ( std::lock_guard l( m_mtx_ ); !contains( h ) )
            {
                NetID id              = find_unique_id();
                host.set_id( id );
                const auto& [ it, _ ] = m_hosts_.emplace( host );
                m_id_wises_.emplace( id, *it );
                return id;
            }

            return -1;
        }

        const NetHost* find( const NetID id ) const
        {
            if ( std::lock_guard l( m_mtx_ ); m_id_wises_.contains( id ) )
            {
                return &m_id_wises_.at( id );
            }

            return nullptr;
        }

        const NetHost* find( const std::array<uint8_t, 4>& addr, const uint16_t port, const eNetSendType type ) const
        {
            const NetHost test( addr, type, port );

            if (std::lock_guard l(m_mtx_); m_hosts_.contains( test ))
            {
                return &(*m_hosts_.find( test ));
            }

            return nullptr;
        }

        [[nodiscard]] bool contains( const NetHost& host ) const
        {
            if ( std::lock_guard l( m_mtx_ ); m_hosts_.contains( host ) )
            {
                return true;
            }

            return false;
        }

        bool remove(const NetID& id)
        {
            if ( std::lock_guard l( m_mtx_ ); find( id ) )
            {
                const NetHost& target = m_id_wises_.at( id );
                m_hosts_.erase( target );
                m_id_wises_.erase( id );
                return true;
            }   

            return false;
        }

    private:
        NetID find_unique_id() const
        {
            NetID i = 0;
            while (true)
            {
                if (m_id_wises_.contains(i))
                {
                    ++i;
                    continue;
                }

                break;
            }
            return i;
        }
    };

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

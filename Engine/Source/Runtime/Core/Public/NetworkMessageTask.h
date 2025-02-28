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
        mutable std::mutex                                  m_mtx_;
        std::unordered_map<NetID, NetHost>                  id_wise_;
        std::unordered_map<uint32_t, NetHost>               addr_wise;

        NetID emplace( NetHost&& h ) 
        {
            NetHost host = std::move( h );

            if ( std::lock_guard l(m_mtx_); !addr_wise.contains( addr_to_int( host.ip ) ) )
            {
                NetID id              = find_unique_id();
                const auto& [ it, _ ] = id_wise_.emplace( id, host );
                addr_wise.insert( { addr_to_int(it->second.ip), it->second } );
                return id;
            }

            return -1;
        }

        const NetHost* const find(NetID id) const
        {
            if ( std::lock_guard l( m_mtx_ ); id_wise_.contains( id ) )
            {
                return &id_wise_.at( id );
            }

            return nullptr;
        }

        const NetHost* const find( const std::array<uint8_t, 4>& addr ) const
        {
            if ( std::lock_guard l( m_mtx_ ); addr_wise.contains( addr_to_int( addr ) ) )
            {
                return &addr_wise.at( addr_to_int( addr ) );
            }

            return nullptr;
        }

        bool remove(const NetID& id)
        {
            if ( std::lock_guard l( m_mtx_ ); id_wise_.contains( id ) )
            {
                NetHost target = id_wise_.at( id );
                addr_wise.erase( addr_to_int( target.ip ) );
                id_wise_.erase( id );
                return true;
            }   

            return false;
        }

    private:
        uint32_t addr_to_int( const std::array<uint8_t, 4>& addr ) const
        {
            return addr[ 0 ] + addr[ 1 ] << 8 + addr[ 2 ] << 16 + addr[ 3 ] << 24;
        }

        NetID find_unique_id() const
        {
            NetID i = 0;
            while (true)
            {
                if (id_wise_.contains(i))
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
        const NetHost* GetNetHost( const std::array<uint8_t, 4>& address );

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

#pragma once
#include <mutex>
#include <unordered_map>
#include <ranges>

#include "NetworkTask.h"
#include "CoreType.h"
#include "TypeLibrary.h"

namespace Engine
{
    struct NetMessageHeaderType;

    struct ENGINE_CORE_API NetworkConsumers final
    {
        NetworkConsumers();
        ~NetworkConsumers();

        INetworkTask* GetConsumer( const NetMessageHeaderType& message ) const;
        INetworkTask* GetConsumer( const HashType hash ) const;
        void          AddConsumer( INetworkTask* task );
        void          Cleanup();

    private:
        mutable std::mutex                                 m_mutex_;
        std::unordered_map<HashType, Unique<INetworkTask>> m_network_consumers_;
    };
}

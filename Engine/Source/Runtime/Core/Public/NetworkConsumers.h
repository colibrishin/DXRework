#pragma once
#include <mutex>
#include <unordered_map>
#include <ranges>
#include "NetworkTask.h"
#include "CoreType.h"

namespace Engine
{
    struct ENGINE_CORE_API NetworkConsumers final
    {
        ~NetworkConsumers() = default;

        INetworkTask* GetConsumer( const RawNetMessage& message ) const;
        INetworkTask* GetConsumer( HashType hash ) const;
        void          AddConsumer( INetworkTask* task );
        void          Cleanup();

    private:
        mutable std::mutex                                         m_mutex_;
        std::unordered_map<HashType, Unique<INetworkTask>> m_network_consumers_;
    };
}
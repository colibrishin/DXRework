#pragma once
#undef SendMessage
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include "CoreType.h"
#include "INetworkAPI.h"
#include "NetworkConsumers.h"
#include "NetworkTask.h"
#include "TypeLibrary.h"

#include "NetworkMessageTask.generated.h"

namespace Engine
{
    ECLASS(virtual)
    struct ENGINE_CORE_API NetworkMessageTask
    {
        GENERATE_BODY
        virtual ~NetworkMessageTask() = default;
        void    Initialize();
        void    Shutdown();
        void    Pool() const;
        [[nodiscard]] bool    IsRunning() const;
        void    PushMessage( Unique<NetMessage>&& message );

        NetworkConsumers& GetConsumers();

    protected:
        void Consume();
        void ConsumeImpl();
        void Send();
        void SendImpl();
        void Receive();
        void ReceiveImpl();
        void Cleanup();
        void SendMessage( Unique<NetMessage>&& message ) const;
        void ReceiveMessage( const RawNetMessage& message );
        void ConsumeMessage( Unique<NetMessage>&& message ) const;

    private:
        std::future<void> m_rx_future_;
        std::future<void> m_tx_future_;
        std::future<void> m_consume_future_;

        mutable std::mutex m_consume_ready_mutex_;
        std::queue<Unique<NetMessage>> m_consume_ready_queue_;

        mutable std::mutex m_tx_mutex_;
        std::queue<Unique<NetMessage>> m_buffer_tx_queue_;
        std::queue<Unique<NetMessage>> m_tx_queue_;

        mutable std::mutex m_rx_mutex_;
        std::queue<RawNetMessage> m_buffer_rx_queue_;
        std::queue<RawNetMessage> m_rx_queue_;

        NetworkConsumers m_consumers_;

        std::atomic<bool> m_tx_running_;
        std::atomic<bool> m_rx_running_;
        std::atomic<bool> m_consume_running_;
    };
}
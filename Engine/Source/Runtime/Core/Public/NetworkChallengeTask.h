#pragma once
#include <unordered_map>
#include <atomic>
#include <future>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include "NetworkTask.h"
#include "DelegateCore.h"

namespace Engine
{
#pragma pack( push, 1 )
    struct ENGINE_CORE_API ChallengeMessage : public NetMessage
    { };
#pragma pack( pop )
}; // namespace Engine

#include "NetworkChallengeTask.generated.h"

namespace Engine
{
    ECLASS( virtual )
    struct NetworkChallengeTask : public NetworkTaskTypeProxy<Engine::ChallengeMessage>
    {
        GENERATE_BODY
        NetworkChallengeTask();

        void Challenge( const NetID new_host );
        void Remove( const NetID removal );

        void Consume( NetMessageDescription&& desc, Unique<NetMessage>&& message ) override;
        bool Convert( const RawNetMessage& message, Unique<NetMessage>& out_mssgae ) override;
        bool Validate( const RawNetMessage& message ) const override;
        void Cleanup() override;

    private:
        bool HaveAck( const NetID id ) const;
        bool NeedAck( const NetID id ) const;
        bool IsWaitDone( const NetID id ) const;
        void KeepChallenge();


        mutable std::recursive_mutex                                     m_mtx_;
        std::unordered_map<NetID, bool>                                  m_challenge_status_{};
        std::unordered_map<NetID, std::chrono::steady_clock::time_point> m_last_challenge_{};

        std::future<void>       m_challenge_task_;
        std::atomic<bool>       m_challenge_task_running_;
        std::mutex              m_wait_mtx_;
        std::condition_variable m_wait_cv_;

        // Registrations on the message task's host events (their listeners use the members above); released in
        // Cleanup(), and declared last so they would also go first on destruction.
        DelegateHandle m_host_added_;
        DelegateHandle m_host_removed_;
    };
}
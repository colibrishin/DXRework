#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "CoreType.h"

namespace Engine::Abstracts
{
    class Entity;
}

namespace Engine
{
    // Calls and destroys a listener's callable without knowing its type. The Delegate templates instantiate one per
    // callable type, in the module that registers the listener; the callable itself lives in a block Core allocates.
    struct DelegateOps
    {
        void ( *invoke )( void* callable, const void* arguments );
        void ( *destroy )( void* callable ) noexcept;
    };

    struct DelegateEntry;

    // The listener list of one Delegate and every operation that changes it. It is created and changed only by Core
    // code, so a module that registers a listener leaves no list, control block or container code of its own behind.
    //
    // Broadcast copies the live entries under a short lock and calls them with no lock held. Each entry counts the
    // calls in progress, so removing it can wait for them: a nested Broadcast, a callback that registers or removes,
    // or callbacks on several threads never deadlock on the list.
    class ENGINE_CORE_API DelegateState : public std::enable_shared_from_this<DelegateState>
    {
    public:
        enum class Threading : uint8_t
        {
            // One thread at a time; the list is not locked. Debug builds report use from another thread.
            Single,
            // Any thread; the list is locked while it is copied or changed, never while listeners run.
            Concurrent,
        };

        static std::shared_ptr<DelegateState> Create( Threading threading );
        ~DelegateState();

        DelegateState( const DelegateState& )            = delete;
        DelegateState& operator=( const DelegateState& ) = delete;

        [[nodiscard]] size_t ListenerCount() const noexcept
        {
            return m_live_.load( std::memory_order_acquire );
        }

        // Memory for a callable that Add() then owns.
        static void* AllocateCallable( size_t size, size_t alignment );
        static void  FreeCallable( void* callable, size_t alignment ) noexcept;

        // Takes ownership of callable. key identifies the registration for Remove and the duplicate check;
        // deduplicate refuses a second live registration with the same owner and key (the callable is destroyed and
        // 0 returned). Returns the registration id otherwise.
        uint64_t Add( const Weak<Abstracts::Entity>& owner,
                      const uintptr_t ( &key )[ 2 ],
                      bool               deduplicate,
                      const DelegateOps* ops,
                      void*              callable,
                      size_t             alignment );

        // Removes the registrations with this key whose owner is owner or can no longer be locked (an owner removing
        // itself in PreDeconstruction has no strong reference left, so its weak pointer is already empty).
        void RemoveByKey( const Weak<Abstracts::Entity>& owner, const uintptr_t ( &key )[ 2 ] );

        // Removes the registrations whose owner is empty and whose key matches (plain functions).
        void RemoveOwnerless( const uintptr_t ( &key )[ 2 ] );

        // Removes one registration. When this returns, the callable is not running on another thread and will not
        // be called again. It is destroyed before returning, or, when the calling thread is inside the callable
        // itself, when that thread's outermost call to it returns.
        void RemoveById( uint64_t id );

        // Calls every live listener with arguments (a tuple of references the templates decode).
        void Invoke( const void* arguments );

    private:
        explicit DelegateState( Threading threading );

        struct LockGuard;

        void Lock() noexcept;
        void Unlock() noexcept;
        bool UnlinkLocked( DelegateEntry* entry ) noexcept;
        void CheckThread() noexcept;

        static void Release( DelegateEntry* entry ) noexcept;
        static void CallEntry( DelegateEntry* entry, const void* arguments );

        const Threading            m_threading_;
        std::atomic_flag           m_lock_ = ATOMIC_FLAG_INIT;
        std::atomic<size_t>        m_live_{ 0 };
        uint64_t                   m_next_id_ = 1;
        std::vector<DelegateEntry*> m_entries_;
        uint32_t                   m_thread_ = 0;
    };

    // Called when a Single delegate is used from another thread than the one that registered its listeners (debug
    // builds). Returns the previous handler; nullptr restores the default (a debugger message).
    using DelegateThreadViolationHandler = void ( * )( const char* message );
    ENGINE_CORE_API DelegateThreadViolationHandler SetDelegateThreadViolationHandler( DelegateThreadViolationHandler handler );

    // Owns one registration from ListenScoped: destroying or resetting it removes the listener, and returns only
    // after calls in progress on other threads have finished. Does nothing when the delegate is already gone.
    class ENGINE_CORE_API DelegateHandle
    {
    public:
        DelegateHandle() = default;
        DelegateHandle( const std::shared_ptr<DelegateState>& state, uint64_t id );
        ~DelegateHandle();

        DelegateHandle( const DelegateHandle& )            = delete;
        DelegateHandle& operator=( const DelegateHandle& ) = delete;
        DelegateHandle( DelegateHandle&& other ) noexcept;
        DelegateHandle& operator=( DelegateHandle&& other ) noexcept;

        void Reset();

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return m_id_ != 0;
        }

    private:
        std::weak_ptr<DelegateState> m_state_;
        uint64_t                     m_id_ = 0;
    };
}

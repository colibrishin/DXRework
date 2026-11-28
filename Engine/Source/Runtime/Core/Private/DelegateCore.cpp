#include "DelegateCore.h"

#include <algorithm>
#include <new>
#include <thread>
#include <windows.h>

#include "Entity.h"

namespace Engine
{
    struct DelegateEntry
    {
        uint64_t                 id = 0;
        Weak<Abstracts::Entity>  owner;
        uintptr_t                key[ 2 ]{};
        const DelegateOps*       ops       = nullptr;
        void*                    callable  = nullptr;
        size_t                   alignment = 0;
        // Keeps the entry's memory: one for the list, one per snapshot holding it, one per remover waiting on it.
        std::atomic<uint32_t>    refs{ 1 };
        // Calls running now (raised just before a call, after which the entry is checked for removal).
        std::atomic<uint32_t>    inflight{ 0 };
        std::atomic<bool>        dead{ false };
        // The callable is destroyed exactly once, by whoever gets here first.
        std::atomic<bool>        destroyed{ false };
        // Set by a listener removing itself after the other threads' calls ended: the thread's outermost call to
        // the entry destroys the callable when it returns.
        bool                     destroy_after_call = false;
    };

    namespace
    {
        // Entries whose callable the current thread is running (innermost last), so a listener that removes itself
        // does not wait for its own call.
        thread_local std::vector<DelegateEntry*> t_running;

        void DefaultThreadViolation( const char* message )
        {
            OutputDebugStringA( message );
        }

        std::atomic<DelegateThreadViolationHandler> g_thread_violation{ &DefaultThreadViolation };

        bool SameKey( const DelegateEntry& entry, const uintptr_t ( &key )[ 2 ] ) noexcept
        {
            return entry.key[ 0 ] == key[ 0 ] && entry.key[ 1 ] == key[ 1 ];
        }

        // Runs the callable's destructor (code of the module that registered it) and frees its block, once.
        void DestroyCallable( DelegateEntry* entry ) noexcept
        {
            if ( !entry->destroyed.exchange( true, std::memory_order_acq_rel ) )
            {
                entry->ops->destroy( entry->callable );
                DelegateState::FreeCallable( entry->callable, entry->alignment );
            }
        }
    }

    DelegateThreadViolationHandler SetDelegateThreadViolationHandler( DelegateThreadViolationHandler handler )
    {
        return g_thread_violation.exchange( handler ? handler : &DefaultThreadViolation );
    }

    std::shared_ptr<DelegateState> DelegateState::Create( const Threading threading )
    {
        return std::shared_ptr<DelegateState>( new DelegateState( threading ) );
    }

    DelegateState::DelegateState( const Threading threading ) : m_threading_( threading ) {}

    DelegateState::~DelegateState()
    {
        // No broadcast is running (each keeps the state alive); snapshots of earlier ones only hold entries.
        for ( DelegateEntry* entry : m_entries_ )
        {
            entry->dead.store( true, std::memory_order_release );
            Release( entry );
        }
    }

    void* DelegateState::AllocateCallable( const size_t size, const size_t alignment )
    {
        return ::operator new( size, std::align_val_t{ alignment } );
    }

    void DelegateState::FreeCallable( void* callable, const size_t alignment ) noexcept
    {
        ::operator delete( callable, std::align_val_t{ alignment } );
    }

    struct DelegateState::LockGuard
    {
        DelegateState& state;

        explicit LockGuard( DelegateState& state ) : state( state )
        {
            state.Lock();
        }

        ~LockGuard()
        {
            state.Unlock();
        }
    };

    void DelegateState::Lock() noexcept
    {
        if ( m_threading_ == Threading::Concurrent )
        {
            while ( m_lock_.test_and_set( std::memory_order_acquire ) )
            {
                m_lock_.wait( true, std::memory_order_relaxed );
            }
        }
    }

    void DelegateState::Unlock() noexcept
    {
        if ( m_threading_ == Threading::Concurrent )
        {
            m_lock_.clear( std::memory_order_release );
            m_lock_.notify_one();
        }
    }

    void DelegateState::CheckThread() noexcept
    {
#if !defined( NDEBUG )
        if ( m_threading_ != Threading::Single )
        {
            return;
        }

        const uint32_t current = GetCurrentThreadId();
        if ( m_entries_.empty() )
        {
            m_thread_ = current;
            return;
        }

        if ( m_thread_ != current )
        {
            g_thread_violation.load()( "Delegate: a single-thread delegate is used from another thread than the one "
                                       "that registered its listeners\n" );
        }
#endif
    }

    bool DelegateState::UnlinkLocked( DelegateEntry* entry ) noexcept
    {
        // seq_cst pairs with Invoke: raise inflight, then read dead. Either the caller sees dead and skips the call,
        // or the remover sees the raised count and waits for the call.
        if ( entry->dead.exchange( true, std::memory_order_seq_cst ) )
        {
            return false;
        }

        std::erase( m_entries_, entry );
        m_live_.fetch_sub( 1, std::memory_order_release );
        return true;
    }

    void DelegateState::Release( DelegateEntry* entry ) noexcept
    {
        if ( entry->refs.fetch_sub( 1, std::memory_order_acq_rel ) == 1 )
        {
            DestroyCallable( entry );
            delete entry;
        }
    }

    uint64_t DelegateState::Add( const Weak<Abstracts::Entity>& owner,
                                 const uintptr_t ( &key )[ 2 ],
                                 const bool         deduplicate,
                                 const DelegateOps* ops,
                                 void*              callable,
                                 const size_t       alignment )
    {
        DelegateEntry* entry = nullptr;
        try
        {
            entry = new DelegateEntry;
        }
        catch ( ... )
        {
            ops->destroy( callable );
            FreeCallable( callable, alignment );
            throw;
        }
        entry->owner     = owner;
        entry->key[ 0 ]  = key[ 0 ];
        entry->key[ 1 ]  = key[ 1 ];
        entry->ops       = ops;
        entry->callable  = callable;
        entry->alignment = alignment;

        {
            LockGuard guard( *this );
            CheckThread();

            bool duplicate = false;
            if ( deduplicate )
            {
                for ( const DelegateEntry* other : m_entries_ )
                {
                    if ( SameKey( *other, key ) && other->owner.owner_equals( owner ) )
                    {
                        duplicate = true;
                        break;
                    }
                }
            }

            if ( !duplicate )
            {
                try
                {
                    m_entries_.push_back( entry );
                }
                catch ( ... )
                {
                    duplicate = true;
                    entry->id = UINT64_MAX;
                }
            }

            if ( !duplicate )
            {
                entry->id = m_next_id_++;
                m_live_.fetch_add( 1, std::memory_order_release );
                return entry->id;
            }
        }

        const bool out_of_memory = entry->id == UINT64_MAX;
        Release( entry );
        if ( out_of_memory )
        {
            throw std::bad_alloc();
        }
        return 0;
    }

    void DelegateState::RemoveByKey( const Weak<Abstracts::Entity>& owner, const uintptr_t ( &key )[ 2 ] )
    {
        std::vector<DelegateEntry*> removed;
        removed.reserve( m_live_.load( std::memory_order_acquire ) );
        {
            LockGuard guard( *this );
            CheckThread();
            for ( size_t i = 0; i < m_entries_.size(); )
            {
                DelegateEntry* entry = m_entries_[ i ];
                if ( SameKey( *entry, key ) && !entry->owner.empty() &&
                     ( entry->owner.owner_equals( owner ) || entry->owner.expired() ) && UnlinkLocked( entry ) )
                {
                    removed.push_back( entry );
                    continue;
                }
                ++i;
            }
        }

        for ( DelegateEntry* entry : removed )
        {
            Release( entry );
        }
    }

    void DelegateState::RemoveOwnerless( const uintptr_t ( &key )[ 2 ] )
    {
        std::vector<DelegateEntry*> removed;
        removed.reserve( m_live_.load( std::memory_order_acquire ) );
        {
            LockGuard guard( *this );
            CheckThread();
            for ( size_t i = 0; i < m_entries_.size(); )
            {
                DelegateEntry* entry = m_entries_[ i ];
                if ( SameKey( *entry, key ) && entry->owner.empty() && UnlinkLocked( entry ) )
                {
                    removed.push_back( entry );
                    continue;
                }
                ++i;
            }
        }

        for ( DelegateEntry* entry : removed )
        {
            Release( entry );
        }
    }

    void DelegateState::RemoveById( const uint64_t id )
    {
        DelegateEntry* entry    = nullptr;
        bool           unlinked = false;

        Lock();
        CheckThread();
        if ( const auto it = std::ranges::find_if( m_entries_, [ id ]( const DelegateEntry* e ) { return e->id == id; } );
             it != m_entries_.end() )
        {
            entry = *it;
            entry->refs.fetch_add( 1, std::memory_order_relaxed );
            unlinked = UnlinkLocked( entry );
        }
        Unlock();

        if ( !entry )
        {
            return;
        }

        if ( unlinked )
        {
            Release( entry );
        }

        // Wait for the calls other threads are making; the ones this thread is inside of finish after we return.
        const auto own = static_cast<uint32_t>( std::ranges::count( t_running, entry ) );
        for ( uint32_t current = entry->inflight.load( std::memory_order_seq_cst ); current > own;
              current          = entry->inflight.load( std::memory_order_seq_cst ) )
        {
            entry->inflight.wait( current, std::memory_order_seq_cst );
        }

        // No other thread is running the callable and none can start it, so it is destroyed before control goes
        // back to the caller: the module that registered it may be unloaded right after. A listener removing itself
        // is still running it; its outermost call destroys it on return (CallEntry).
        if ( own == 0 )
        {
            DestroyCallable( entry );
        }
        else
        {
            entry->destroy_after_call = true;
        }

        Release( entry );
    }

    void DelegateState::Invoke( const void* arguments )
    {
        // A listener may destroy the delegate's owner (and so this state) while the broadcast is running.
        const std::shared_ptr<DelegateState> keep_alive = shared_from_this();

        constexpr size_t inline_capacity = 16;
        DelegateEntry*                inline_entries[ inline_capacity ];
        std::vector<DelegateEntry*>   heap_entries;
        DelegateEntry**               snapshot = inline_entries;
        size_t                        count    = 0;

        {
            LockGuard guard( *this );
            CheckThread();
            count = m_entries_.size();
            if ( count > inline_capacity )
            {
                heap_entries.resize( count );
                snapshot = heap_entries.data();
            }
            for ( size_t i = 0; i < count; ++i )
            {
                DelegateEntry* entry = m_entries_[ i ];
                entry->refs.fetch_add( 1, std::memory_order_relaxed );
                snapshot[ i ] = entry;
            }
        }

        size_t next = 0;
        try
        {
            for ( ; next < count; ++next )
            {
                DelegateEntry* entry = snapshot[ next ];

                if ( !entry->owner.empty() && entry->owner.expired() )
                {
                    bool unlinked;
                    {
                        LockGuard guard( *this );
                        unlinked = UnlinkLocked( entry );
                    }
                    if ( unlinked )
                    {
                        Release( entry );
                    }
                }
                else
                {
                    CallEntry( entry, arguments );
                }

                Release( entry );
            }
        }
        catch ( ... )
        {
            // Release the snapshot's references, the entry whose call threw included.
            for ( ; next < count; ++next )
            {
                Release( snapshot[ next ] );
            }
            throw;
        }
    }

    void DelegateState::CallEntry( DelegateEntry* entry, const void* arguments )
    {
        // Raise inflight, then check for removal (seq_cst, paired with UnlinkLocked): a removed entry is never called
        // after its remover saw no call in progress.
        entry->inflight.fetch_add( 1, std::memory_order_seq_cst );

        struct Leave
        {
            DelegateEntry* entry;

            ~Leave()
            {
                entry->inflight.fetch_sub( 1, std::memory_order_seq_cst );
                if ( entry->dead.load( std::memory_order_seq_cst ) )
                {
                    // A remover may be waiting for this call to end. It waits for the count to drop to the calls it
                    // is itself inside of, which can be more than zero, so every decrement wakes it.
                    entry->inflight.notify_all();
                }
            }
        } leave{ entry };

        if ( entry->dead.load( std::memory_order_seq_cst ) )
        {
            return;
        }

        t_running.push_back( entry );
        struct FinishCall
        {
            DelegateEntry* entry;

            ~FinishCall()
            {
                t_running.pop_back();
                if ( entry->destroy_after_call && std::ranges::find( t_running, entry ) == t_running.end() )
                {
                    // This thread removed the listener from inside it, after the other threads' calls ended.
                    DestroyCallable( entry );
                }
            }
        } finish_call{ entry };
        entry->ops->invoke( entry->callable, arguments );
    }

    DelegateHandle::DelegateHandle( const std::shared_ptr<DelegateState>& state, const uint64_t id )
        : m_state_( state ),
          m_id_( id ) {}

    DelegateHandle::~DelegateHandle()
    {
        Reset();
    }

    DelegateHandle::DelegateHandle( DelegateHandle&& other ) noexcept
        : m_state_( std::move( other.m_state_ ) ),
          m_id_( std::exchange( other.m_id_, 0 ) ) {}

    DelegateHandle& DelegateHandle::operator=( DelegateHandle&& other ) noexcept
    {
        if ( this != &other )
        {
            Reset();
            m_state_ = std::move( other.m_state_ );
            m_id_    = std::exchange( other.m_id_, 0 );
        }
        return *this;
    }

    void DelegateHandle::Reset()
    {
        if ( m_id_ == 0 )
        {
            return;
        }

        if ( const std::shared_ptr<DelegateState> state = m_state_.lock() )
        {
            state->RemoveById( m_id_ );
        }
        m_state_.reset();
        m_id_ = 0;
    }
}

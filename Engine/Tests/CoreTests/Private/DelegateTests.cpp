#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <latch>
#include <thread>
#include <vector>

#include "Delegation.hpp"
#include "TestListener.h"

// Delegate API added with the rework: handles, ownerless listeners, copy/move rules, thread reporting, and
// ConcurrentDelegate's guarantees for listeners running on other threads.

using Engine::Tests::TestListener;

namespace
{
    Engine::Strong<TestListener> MakeListener()
    {
        return make_managed_shared<TestListener>();
    }

    Engine::Weak<TestListener> WeakOf( const Engine::Strong<TestListener>& listener )
    {
        return listener;
    }

    int  plain_calls = 0;
    void PlainListener( int )
    {
        ++plain_calls;
    }

    int  thread_violations = 0;
    void CountViolation( const char* )
    {
        ++thread_violations;
    }
}

TEST( Delegate, ListenWithoutKeepingAnythingStaysRegistered )
{
    Delegate<int> delegate;
    int           calls = 0;
    delegate.Listen( [ & ]( int ) { ++calls; } );

    delegate.Broadcast( 1 );
    delegate.Broadcast( 1 );

    EXPECT_EQ( calls, 2 );
}

TEST( Delegate, HandleRemovesItsListener )
{
    Delegate<int> delegate;
    int           calls  = 0;
    auto          handle = delegate.ListenScoped( [ & ]( int ) { ++calls; } );

    delegate.Broadcast( 1 );
    handle.Reset();
    delegate.Broadcast( 1 );

    EXPECT_EQ( calls, 1 );
    EXPECT_EQ( delegate.ListenerCount(), 0u );
}

TEST( Delegate, HandleOutlivingTheDelegateDoesNothing )
{
    Engine::DelegateHandle handle;
    {
        Delegate<int> delegate;
        handle = delegate.ListenScoped( []( int ) {} );
    }
    handle.Reset();
    EXPECT_FALSE( handle );
}

TEST( Delegate, PlainFunctionIsRemovedByValue )
{
    Delegate<int> delegate;
    plain_calls = 0;
    delegate.Listen( &PlainListener );
    delegate.Broadcast( 1 );
    delegate.Remove( &PlainListener );
    delegate.Broadcast( 1 );

    EXPECT_EQ( plain_calls, 1 );
}

TEST( Delegate, SameMemberListenerIsRegisteredOnce )
{
    Delegate<int> delegate;
    const auto    a = MakeListener();
    delegate.Listen( WeakOf( a ), &TestListener::OnSignal );
    delegate.Listen( WeakOf( a ), &TestListener::OnSignal );

    delegate.Broadcast( 1 );

    EXPECT_EQ( a->calls, 1 );
}

TEST( Delegate, ExpiredOwnerIsDroppedFromTheList )
{
    Delegate<int> delegate;
    auto          a = MakeListener();
    delegate.Listen( WeakOf( a ), &TestListener::OnSignal );
    a.reset();

    delegate.Broadcast( 1 );

    EXPECT_EQ( delegate.ListenerCount(), 0u );
}

TEST( Delegate, CopyAssignmentKeepsTheTargetsListeners )
{
    Delegate<int> source;
    Delegate<int> target;
    int           source_calls = 0;
    int           target_calls = 0;
    source.Listen( [ & ]( int ) { ++source_calls; } );
    target.Listen( [ & ]( int ) { ++target_calls; } );

    target = source;
    target.Broadcast( 1 );

    EXPECT_EQ( source_calls, 0 );
    EXPECT_EQ( target_calls, 1 );
}

TEST( Delegate, MoveCarriesListenersAndHandles )
{
    Delegate<int> source;
    int           calls  = 0;
    auto          handle = source.ListenScoped( [ & ]( int ) { ++calls; } );

    Delegate<int> moved = std::move( source );
    moved.Broadcast( 1 );
    handle.Reset();
    moved.Broadcast( 1 );

    EXPECT_EQ( calls, 1 );
}

TEST( Delegate, UseFromAnotherThreadIsReported )
{
#if defined( NDEBUG )
    GTEST_SKIP() << "checked in debug builds only";
#else
    thread_violations   = 0;
    const auto previous = Engine::SetDelegateThreadViolationHandler( &CountViolation );

    Delegate<int> delegate;
    delegate.Listen( []( int ) {} );
    std::thread( [ & ] { delegate.Broadcast( 1 ); } ).join();

    Engine::SetDelegateThreadViolationHandler( previous );
    EXPECT_EQ( thread_violations, 1 );
#endif
}

TEST( ConcurrentDelegate, HandleResetWaitsForTheCallInProgress )
{
    ConcurrentDelegate<int> delegate;
    std::latch              entered( 1 );
    std::latch              release( 1 );
    auto handle = delegate.ListenScoped( [ & ]( int )
    {
        entered.count_down();
        release.wait();
    } );

    std::thread broadcaster( [ & ] { delegate.Broadcast( 1 ); } );
    entered.wait();

    std::atomic<bool> reset_done = false;
    std::thread       remover( [ & ]
    {
        handle.Reset();
        reset_done = true;
    } );

    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    EXPECT_FALSE( reset_done.load() );

    release.count_down();
    remover.join();
    broadcaster.join();
    EXPECT_TRUE( reset_done.load() );
}

TEST( ConcurrentDelegate, ListenerResettingItsOwnHandleIsNotCalledAgain )
{
    ConcurrentDelegate<int> delegate;
    int                     calls = 0;
    Engine::DelegateHandle  handle;
    handle = delegate.ListenScoped( [ & ]( int )
    {
        ++calls;
        handle.Reset();
    } );

    delegate.Broadcast( 1 );
    std::thread( [ & ] { delegate.Broadcast( 1 ); } ).join();

    EXPECT_EQ( calls, 1 );
}

TEST( ConcurrentDelegate, NestedBroadcastWhileAnotherThreadRegistersDoesNotDeadlock )
{
    ConcurrentDelegate<int> first;
    ConcurrentDelegate<int> second;
    std::atomic<bool>       registering = false;
    std::atomic<bool>       stop        = false;
    std::atomic<int>        nested      = 0;

    auto outer = first.ListenScoped( [ & ]( int value )
    {
        if ( value != 1 )
        {
            ++nested;
            return;
        }
        while ( !registering )
        {
            std::this_thread::yield();
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
        first.Broadcast( 2 );
        second.Broadcast( 2 );
    } );
    auto inner = second.ListenScoped( [ & ]( int ) { ++nested; } );

    std::thread registrar( [ & ]
    {
        registering = true;
        while ( !stop )
        {
            auto handle = first.ListenScoped( []( int ) {} );
        }
    } );

    auto run = std::async( std::launch::async, [ & ] { first.Broadcast( 1 ); } );
    const bool finished = run.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready;
    stop = true;
    registrar.join();

    if ( !finished )
    {
        // The blocked broadcast still uses this frame's locals: report and end the process instead of hanging.
        std::fprintf( stderr, "nested broadcast blocked\n" );
        std::fflush( stderr );
        std::_Exit( 3 );
    }
    EXPECT_EQ( nested.load(), 2 );
}

TEST( ConcurrentDelegate, ListenerResettingAnotherListenersHandleDuringTheBroadcast )
{
    // Whichever runs first, the broadcast ends, and b is not called after a reset its handle.
    ConcurrentDelegate<int> delegate;
    Engine::DelegateHandle  b;
    int                     b_calls_after_reset = 0;
    bool                    reset               = false;
    auto                    a = delegate.ListenScoped( [ & ]( int )
    {
        b.Reset();
        reset = true;
    } );
    b = delegate.ListenScoped( [ & ]( int )
    {
        if ( reset )
        {
            ++b_calls_after_reset;
        }
    } );

    auto run = std::async( std::launch::async, [ & ] { delegate.Broadcast( 1 ); } );
    if ( run.wait_for( std::chrono::seconds( 5 ) ) != std::future_status::ready )
    {
        std::fprintf( stderr, "broadcast blocked\n" );
        std::fflush( stderr );
        std::_Exit( 3 );
    }
    EXPECT_EQ( b_calls_after_reset, 0 );
    EXPECT_EQ( delegate.ListenerCount(), 1u );
}

TEST( ConcurrentDelegate, ListenerResettingItsOwnHandleInsideANestedBroadcast )
{
    // a broadcasts again; b, still waiting its turn in the outer broadcast, removes itself in the inner one and is
    // not called by the outer one afterwards.
    ConcurrentDelegate<int> delegate;
    Engine::DelegateHandle  b;
    int                     b_calls = 0;
    auto                    a = delegate.ListenScoped( [ & ]( const int value )
    {
        if ( value == 1 )
        {
            delegate.Broadcast( 2 );
        }
    } );
    b = delegate.ListenScoped( [ & ]( int )
    {
        ++b_calls;
        b.Reset();
    } );

    auto run = std::async( std::launch::async, [ & ] { delegate.Broadcast( 1 ); } );
    if ( run.wait_for( std::chrono::seconds( 5 ) ) != std::future_status::ready )
    {
        std::fprintf( stderr, "broadcast blocked\n" );
        std::fflush( stderr );
        std::_Exit( 3 );
    }
    EXPECT_EQ( b_calls, 1 );
    EXPECT_EQ( delegate.ListenerCount(), 1u );
}

TEST( ConcurrentDelegate, ListenerResettingItsOwnHandleWhileAnotherThreadIsInsideIt )
{
    // The reset waits for the other thread's call, and ends when that call does.
    ConcurrentDelegate<int> delegate;
    Engine::DelegateHandle  handle;
    std::latch              both_inside( 2 );
    std::atomic<bool>       resetting = false;
    std::atomic<int>        calls     = 0;
    handle = delegate.ListenScoped( [ & ]( const int value )
    {
        ++calls;
        both_inside.arrive_and_wait();
        if ( value == 1 )
        {
            resetting = true;
            handle.Reset();
        }
        else
        {
            while ( !resetting )
            {
                std::this_thread::yield();
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }
    } );

    auto other = std::async( std::launch::async, [ & ] { delegate.Broadcast( 2 ); } );
    auto self  = std::async( std::launch::async, [ & ] { delegate.Broadcast( 1 ); } );
    if ( self.wait_for( std::chrono::seconds( 5 ) ) != std::future_status::ready )
    {
        std::fprintf( stderr, "reset blocked\n" );
        std::fflush( stderr );
        std::_Exit( 3 );
    }
    other.wait();
    EXPECT_EQ( calls.load(), 2 );
    EXPECT_EQ( delegate.ListenerCount(), 0u );
}

TEST( ConcurrentDelegate, SelfResetDestroysTheCallableBeforeTheBroadcastReturns )
{
    // Another thread's broadcast still holds the listener in its snapshot; the callable is destroyed anyway when the
    // broadcast that removed it returns, so the registering module could be unloaded then.
    ConcurrentDelegate<int> delegate;
    std::atomic<bool>       destroyed = false;
    std::atomic<bool>       other_inside = false;
    std::latch              release_other( 1 );
    int                     self_calls = 0;

    auto blocker = delegate.ListenScoped( [ & ]( const int value )
    {
        if ( value == 2 )
        {
            other_inside = true;
            release_other.wait();
        }
    } );

    Engine::DelegateHandle handle;
    {
        std::shared_ptr<int> token( new int( 0 ), [ & ]( const int* p )
        {
            delete p;
            destroyed = true;
        } );
        handle = delegate.ListenScoped( [ &, token ]( int )
        {
            ++self_calls;
            handle.Reset();
        } );
    }

    auto other = std::async( std::launch::async, [ & ] { delegate.Broadcast( 2 ); } );
    while ( !other_inside )
    {
        std::this_thread::yield();
    }

    delegate.Broadcast( 1 );
    EXPECT_TRUE( destroyed.load() );

    release_other.count_down();
    other.wait();
    EXPECT_EQ( self_calls, 1 );
    EXPECT_EQ( delegate.ListenerCount(), 1u );
}

TEST( ConcurrentDelegate, NoCallAfterHandleResetUnderLoad )
{
    // Slots are never freed, so a call after its handle was reset is counted instead of touching freed memory.
    struct Slot
    {
        std::atomic<bool> closed = false;
    };
    constexpr int                      registrations_per_thread = 5000;
    std::vector<std::unique_ptr<Slot>> slots;
    for ( int i = 0; i < registrations_per_thread * 2; ++i )
    {
        slots.push_back( std::make_unique<Slot>() );
    }

    ConcurrentDelegate<int> delegate;
    std::atomic<bool>       stop       = false;
    std::atomic<int>        violations = 0;

    std::vector<std::thread> broadcasters;
    for ( int i = 0; i < 6; ++i )
    {
        broadcasters.emplace_back( [ & ]
        {
            while ( !stop )
            {
                delegate.Broadcast( 1 );
            }
        } );
    }

    std::vector<std::thread> registrars;
    for ( int t = 0; t < 2; ++t )
    {
        registrars.emplace_back( [ &, t ]
        {
            for ( int i = 0; i < registrations_per_thread; ++i )
            {
                Slot* slot   = slots[ t * registrations_per_thread + i ].get();
                auto  handle = delegate.ListenScoped( [ slot, &violations ]( int )
                {
                    if ( slot->closed.load( std::memory_order_acquire ) )
                    {
                        ++violations;
                    }
                } );
                handle.Reset();
                slot->closed.store( true, std::memory_order_release );
            }
        } );
    }

    for ( std::thread& registrar : registrars )
    {
        registrar.join();
    }
    stop = true;
    for ( std::thread& broadcaster : broadcasters )
    {
        broadcaster.join();
    }

    EXPECT_EQ( violations.load(), 0 );
    EXPECT_EQ( delegate.ListenerCount(), 0u );
}

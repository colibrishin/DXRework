#include <gtest/gtest.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "FenceRetireQueue.h"

using Engine::FenceRetireQueue;

TEST( FenceRetireQueue, RunsWorkUpToTheCompletedValueInOrder )
{
    FenceRetireQueue queue;
    std::vector<int> ran;
    queue.Push( 1, [ & ] { ran.push_back( 1 ); } );
    queue.Push( 2, [ & ] { ran.push_back( 2 ); } );
    queue.Push( 2, [ & ] { ran.push_back( 20 ); } );
    queue.Push( 5, [ & ] { ran.push_back( 5 ); } );

    EXPECT_EQ( queue.Retire( 0 ), 0u );
    EXPECT_TRUE( ran.empty() );
    EXPECT_EQ( queue.Retire( 2 ), 3u );
    EXPECT_EQ( ran, ( std::vector<int>{ 1, 2, 20 } ) );
    EXPECT_EQ( queue.OldestPending(), 5u );
    EXPECT_FALSE( queue.Empty() );
    EXPECT_EQ( queue.Retire( 4 ), 0u );
    EXPECT_EQ( queue.Retire( 9 ), 1u );
    EXPECT_EQ( ran.back(), 5 );
    EXPECT_TRUE( queue.Empty() );
    EXPECT_EQ( queue.OldestPending(), 0u );
}

TEST( FenceRetireQueue, WorkMayPushMoreWork )
{
    // Work runs outside the queue's lock: it can queue more (e.g. a release that frees a resource holding another).
    FenceRetireQueue queue;
    int              ran = 0;
    queue.Push( 1, [ & ]
    {
        ++ran;
        queue.Push( 3, [ & ] { ran += 10; } );
    } );
    EXPECT_EQ( queue.Retire( 1 ), 1u );
    EXPECT_EQ( ran, 1 );
    EXPECT_EQ( queue.OldestPending(), 3u );
    EXPECT_EQ( queue.Retire( 3 ), 1u );
    EXPECT_EQ( ran, 11 );
}

TEST( FenceRetireQueue, RetireAllRunsEverything )
{
    FenceRetireQueue queue;
    int              ran = 0;
    queue.Push( 7, [ & ] { ++ran; } );
    queue.Push( 8, [ & ] { ++ran; } );
    EXPECT_EQ( queue.RetireAll(), 2u );
    EXPECT_EQ( ran, 2 );
    EXPECT_TRUE( queue.Empty() );
}

TEST( FenceRetireQueue, ConcurrentPushAndRetireRunEachWorkOnce )
{
    FenceRetireQueue      queue;
    std::atomic<uint64_t> next{ 0 };
    std::atomic<int>      ran{ 0 };
    std::mutex            order; // pushes must be in fence order: the caller signals and pushes under one lock
    constexpr int         per_thread = 2000;

    std::vector<std::thread> pushers;
    for ( int t = 0; t < 4; ++t )
    {
        pushers.emplace_back( [ & ]
        {
            for ( int i = 0; i < per_thread; ++i )
            {
                std::lock_guard l( order );
                queue.Push( ++next, [ & ] { ++ran; } );
            }
        } );
    }
    std::atomic<bool> done{ false };
    std::thread       retirer( [ & ]
    {
        while ( !done )
        {
            queue.Retire( next.load() );
        }
    } );
    for ( std::thread& t : pushers )
    {
        t.join();
    }
    done = true;
    retirer.join();
    queue.RetireAll();
    EXPECT_EQ( ran.load(), 4 * per_thread );
}

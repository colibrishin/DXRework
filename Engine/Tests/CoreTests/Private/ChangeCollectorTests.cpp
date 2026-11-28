#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

#include "ChangeCollector.h"

using Engine::ChangeCollector;
using Engine::ChangeEvent;
using Engine::eChange;

namespace
{
    void Emit( const ChangeEvent& event )
    {
        if ( Engine::ChangeSignalActive() )
        {
            Engine::EmitChange( "test", event );
        }
    }
}

TEST( ChangeSignal, InactiveWithoutACollector )
{
    EXPECT_FALSE( Engine::ChangeSignalActive() );
    {
        ChangeCollector collector;
        EXPECT_TRUE( Engine::ChangeSignalActive() );
    }
    EXPECT_FALSE( Engine::ChangeSignalActive() );
}

TEST( ChangeCollector, DrainHandsOverTheEventsAndStartsANewEpoch )
{
    ChangeCollector          collector;
    std::vector<ChangeEvent> events;
    const uint64_t           epoch_before = Engine::ChangeEpoch();

    Emit( { 1, 10, 0, eChange::Transform } );
    Emit( { 1, 10, 0, eChange::Active } );
    Emit( { 1, 10, 20, eChange::Modified } );
    Emit( { 1, 11, 21, eChange::Added } );
    Emit( { 1, 0, 0, eChange::SceneReset } );

    EXPECT_EQ( collector.Drain( events ), 1u );
    EXPECT_EQ( events.size(), 5u );
    EXPECT_EQ( Engine::ChangeEpoch(), epoch_before + 1 );

    const Engine::MergedChanges merged = Engine::MergeChanges( events );
    ASSERT_EQ( merged.objects.size(), 1u );
    EXPECT_EQ( merged.objects.at( 10 ), eChange::Transform | eChange::Active );
    ASSERT_EQ( merged.components.size(), 2u );
    EXPECT_EQ( merged.components.at( 20 ), static_cast<uint16_t>( eChange::Modified ) );
    EXPECT_EQ( merged.component_owners.at( 21 ), 11u );
    EXPECT_EQ( merged.scenes.at( 1 ), static_cast<uint16_t>( eChange::SceneReset ) );

    EXPECT_EQ( collector.Drain( events ), 2u );
    EXPECT_TRUE( events.empty() );
}

TEST( ChangeCollector, ANewCollectorStartsANewEpoch )
{
    // A source stamps the epoch when it sends (Transform does). After that collector goes away without a drain, the
    // next collector must not see the old stamp as current, or the source would stay silent.
    uint64_t stamp;
    {
        ChangeCollector first;
        stamp = Engine::ChangeEpoch();
    }
    // Creating a collector alone moves the epoch (whatever happened when the previous one went away).
    const uint64_t before_second = Engine::ChangeEpoch();
    ChangeCollector second;
    EXPECT_NE( Engine::ChangeEpoch(), before_second );
    EXPECT_NE( Engine::ChangeEpoch(), stamp );
}

TEST( ChangeCollector, CollectsEventsFromSeveralThreads )
{
    ChangeCollector          collector;
    std::vector<std::thread> threads;
    for ( uint32_t t = 0; t < 4; ++t )
    {
        threads.emplace_back( [ t ]
        {
            for ( uint32_t i = 0; i < 1000; ++i )
            {
                Emit( { 1, 1 + t * 1000 + i, 0, eChange::Transform } );
            }
        } );
    }
    for ( std::thread& thread : threads )
    {
        thread.join();
    }
    std::vector<ChangeEvent> events;
    collector.Drain( events );
    EXPECT_EQ( Engine::MergeChanges( events ).objects.size(), 4000u );
}

TEST( ChangeCollector, MutatedSiteIsDroppedOnlyWhenNamed )
{
    // KOLIBRI_DIRTY_MUTATE is read once per process; in the test process it is unset, so nothing is dropped.
    ChangeCollector          collector;
    std::vector<ChangeEvent> events;
    Engine::EmitChange( "transform", { 1, 2, 0, eChange::Transform } );
    collector.Drain( events );
    EXPECT_EQ( events.size(), 1u );
}

// Cost per event (run with --gtest_also_run_disabled_tests). Prints, does not judge.
TEST( ChangeSignalCost, DISABLED_PerEvent )
{
    constexpr int n = 1000000;
    using clock     = std::chrono::steady_clock;
    const auto ns   = []( const clock::duration d ) { return std::chrono::duration<double, std::nano>( d ).count() / n; };

    auto t0 = clock::now();
    for ( int i = 0; i < n; ++i )
    {
        Emit( { 1, static_cast<Engine::GlobalEntityID>( i ), 0, eChange::Transform } );
    }
    const double inactive = ns( clock::now() - t0 );

    std::mutex               mutex;
    std::vector<ChangeEvent> plain;
    plain.reserve( n );
    t0 = clock::now();
    for ( int i = 0; i < n; ++i )
    {
        std::lock_guard lock( mutex );
        plain.push_back( { 1, static_cast<Engine::GlobalEntityID>( i ), 0, eChange::Transform } );
    }
    const double locked_push = ns( clock::now() - t0 );

    ChangeCollector          collector;
    std::vector<ChangeEvent> events;
    for ( int round = 0; round < 2; ++round ) // the second round reuses the drained capacity
    {
        t0 = clock::now();
        for ( int i = 0; i < n; ++i )
        {
            Emit( { 1, static_cast<Engine::GlobalEntityID>( i ), 0, eChange::Transform } );
        }
        const double collected = ns( clock::now() - t0 );
        collector.Drain( events );
        std::printf( "round %d: inactive %.1f ns, locked push %.1f ns, collected %.1f ns per event\n", round, inactive,
                     locked_push, collected );
    }
}

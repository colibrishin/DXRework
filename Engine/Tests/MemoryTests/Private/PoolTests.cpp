#include <gtest/gtest.h>

#include "PoolTestAccess.h"

#include <thread>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

using Engine::Strong;
using Engine::Weak;

namespace pool_tests
{
    constexpr int kGrowth = 2600; // > 10 chunks of 256 slots
}
using pool_tests::kGrowth;

TEST( PoolStorage, AddressesStayWhilePoolGrows )
{
    using T = Item<1>;
    std::vector<Strong<T>> items;
    items.push_back( make_managed_shared<T>( 7 ) );

    const pool_core* core   = pool_test_access::core<T>();
    const void*      object = items[ 0 ].get();
    const void*      start  = pool_test_access::start( core );
    const void*      mask   = pool_test_access::mask( core );
    const void*      gens   = pool_test_access::generations( core );

    for ( int i = 0; i < kGrowth; ++i )
    {
        items.push_back( make_managed_shared<T>( i ) );
    }

    ASSERT_GT( pool_test_access::committed_chunks( core ), 10u );
    EXPECT_EQ( items[ 0 ].get(), object );
    EXPECT_EQ( pool_test_access::start( core ), start );
    EXPECT_EQ( pool_test_access::mask( core ), mask );
    EXPECT_EQ( pool_test_access::generations( core ), gens );
    EXPECT_EQ( items[ 0 ]->value, 7 );
    EXPECT_EQ( items[ kGrowth ]->value, kGrowth - 1 );
}

namespace pool_tests
{
    // Grows its own pool from another thread while it is being constructed (after its slot was reserved).
    struct GrowsOnOtherThread
    {
        inline static std::vector<Strong<GrowsOnOtherThread>>* sink = nullptr;
        int value = 0;

        GrowsOnOtherThread() = default;
        explicit GrowsOnOtherThread( const int v )
        {
            std::thread( [] {
                for ( int i = 0; i < 600; ++i ) { sink->push_back( make_managed_shared<GrowsOnOtherThread>() ); }
            } ).join();
            value = v;
        }
    };

    // Grows its own pool on the same thread while it is being constructed.
    struct GrowsReentrant
    {
        int value = 0;

        GrowsReentrant() = default;
        GrowsReentrant( const int v, std::vector<Strong<GrowsReentrant>>* out, const int n )
        {
            for ( int i = 0; i < n; ++i ) { out->push_back( make_managed_shared<GrowsReentrant>() ); }
            value = v;
        }
    };
}
using pool_tests::GrowsOnOtherThread;
using pool_tests::GrowsReentrant;

TEST( PoolStorage, ConstructionSurvivesGrowthOnAnotherThread )
{
    std::vector<Strong<GrowsOnOtherThread>> others;
    GrowsOnOtherThread::sink = &others;
    const Strong<GrowsOnOtherThread> item = make_managed_shared<GrowsOnOtherThread>( 42 );
    GrowsOnOtherThread::sink = nullptr;

    EXPECT_EQ( others.size(), 600u );
    EXPECT_EQ( item->value, 42 );
}

TEST( PoolStorage, ConstructionSurvivesReentrantGrowth )
{
    std::vector<Strong<GrowsReentrant>> others;
    const Strong<GrowsReentrant> item = make_managed_shared<GrowsReentrant>( 42, &others, 600 );

    EXPECT_EQ( others.size(), 600u );
    EXPECT_EQ( item->value, 42 );
}

TEST( PoolStorage, CoreLivesOutsideEveryModuleImage )
{
    (void)make_managed_shared<Item<2>>();
    HMODULE    owner     = nullptr;
    const BOOL in_module = GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>( pool_test_access::core<Item<2>>() ), &owner );
    EXPECT_FALSE( in_module );
}

TEST( PoolStorage, FreedSlotIsReusedAndOldWeakExpires )
{
    using T = Item<3>;
    Strong<T>   first = make_managed_shared<T>( 1 );
    const void* addr  = first.get();
    Weak<T>     weak  = first;

    first.reset();
    EXPECT_TRUE( weak.expired() );
    EXPECT_FALSE( weak.lock() );

    const Strong<T> second = make_managed_shared<T>( 2 );
    EXPECT_EQ( second.get(), addr ); // same slot, new generation
    EXPECT_FALSE( weak.lock() );     // the old weak pointer does not see the new object
}

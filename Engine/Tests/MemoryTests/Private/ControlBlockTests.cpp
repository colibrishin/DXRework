#include <gtest/gtest.h>

#include "PoolTestAccess.h"

#include <atomic>
#include <thread>
#include <vector>

using Engine::Strong;
using Engine::Weak;

// Control blocks of managed pointers come from one fixed-size pool in Memory.dll (not the default heap).

TEST( ControlBlockPool, EachAllocationTakesOneBlockSharedByCopies )
{
    const size_t before = pool_test_access::live_control_blocks();
    {
        const Strong<Item<40>> a = make_managed_shared<Item<40>>( 1 );
        const Strong<Item<40>> b = a;
        const Weak<Item<40>>   w = a;
        const Strong<Item<41>> c = make_managed_shared<Item<41>>( 2 );
        EXPECT_EQ( pool_test_access::live_control_blocks(), before + 2 );
    }
    EXPECT_EQ( pool_test_access::live_control_blocks(), before );
}

TEST( ControlBlockPool, BlockLivesUntilTheLastWeakPointer )
{
    const size_t     before = pool_test_access::live_control_blocks();
    Strong<Item<42>> item   = make_managed_shared<Item<42>>( 1 );
    Weak<Item<42>>   weak   = item;

    item.reset(); // object freed, block still holds the weak count
    EXPECT_EQ( pool_test_access::live_control_blocks(), before + 1 );
    EXPECT_FALSE( weak.lock() );

    weak.reset();
    EXPECT_EQ( pool_test_access::live_control_blocks(), before );
}

TEST( ControlBlockPool, FreedBlocksAreReused )
{
    for ( int i = 0; i < 10; ++i ) { (void)make_managed_shared<Item<43>>(); } // warm up the free list
    const size_t used = pool_test_access::used_control_block_bytes();
    for ( int i = 0; i < 1000; ++i ) { (void)make_managed_shared<Item<43>>( i ); }
    EXPECT_EQ( pool_test_access::used_control_block_bytes(), used );
}

TEST( ControlBlockPool, ConcurrentCreateCopyAndRelease )
{
    const size_t before = pool_test_access::live_control_blocks();
    {
        std::vector<Strong<Item<44>>> shared;
        for ( int i = 0; i < 256; ++i ) { shared.push_back( make_managed_shared<Item<44>>( i ) ); }

        std::atomic<int>         sum{ 0 };
        std::vector<std::thread> threads;
        for ( int t = 0; t < 8; ++t )
        {
            threads.emplace_back( [ &, t ] {
                std::vector<Strong<Item<44>>> mine;
                for ( int i = 0; i < 20000; ++i )
                {
                    mine.push_back( make_managed_shared<Item<44>>( i ) );
                    const Weak<Item<44>> weak = shared[ ( i + t ) % shared.size() ];
                    if ( const Strong<Item<44>> locked = weak.lock() ) { sum += locked->value & 1; }
                    if ( mine.size() > 64 ) { mine.erase( mine.begin(), mine.begin() + 32 ); }
                }
            } );
        }
        for ( std::thread& t : threads ) { t.join(); }
        EXPECT_GT( sum.load(), 0 );
    }
    EXPECT_EQ( pool_test_access::live_control_blocks(), before );
}

TEST( ControlBlockPoolDeathTest, FreeingABlockTwice )
{
    EXPECT_EXIT(
            {
                void* block = pool_test_access::allocate_control_block();
                pool_test_access::deallocate_control_block( block );
                pool_test_access::deallocate_control_block( block );
                std::exit( 0 );
            },
            testing::ExitedWithCode( static_cast<int>( 0xC0000409u ) ), "" );
}

TEST( ControlBlockPoolDeathTest, FreeingAPointerFromOutsideThePool )
{
    EXPECT_EXIT(
            {
                (void)pool_test_access::allocate_control_block();
                alignas( 16 ) char foreign[ 96 ]{};
                pool_test_access::deallocate_control_block( foreign );
                std::exit( 0 );
            },
            testing::ExitedWithCode( static_cast<int>( 0xC0000409u ) ), "" );
}

TEST( ControlBlockPoolDeathTest, FreeingAnInteriorPointer )
{
    EXPECT_EXIT(
            {
                char* block = static_cast<char*>( pool_test_access::allocate_control_block() );
                pool_test_access::deallocate_control_block( block + 16 );
                std::exit( 0 );
            },
            testing::ExitedWithCode( static_cast<int>( 0xC0000409u ) ), "" );
}

TEST( ControlBlockPoolDeathTest, WriteIntoAFreedBlock )
{
    EXPECT_EXIT(
            {
                void* block = pool_test_access::allocate_control_block();
                pool_test_access::deallocate_control_block( block );
                static_cast<void**>( block )[ 0 ] = reinterpret_cast<void*>( 0x1234 ); // stale write: free list link
                (void)pool_test_access::allocate_control_block();
                std::exit( 0 );
            },
            testing::ExitedWithCode( static_cast<int>( 0xC0000409u ) ), "" );
}

// Managed pointers that outlive cleanup (globals destroyed at exit) still release their control block safely.
TEST( ControlBlockPoolDeathTest, ReleasingAfterCleanup )
{
    EXPECT_EXIT(
            {
                const size_t     before = pool_test_access::live_control_blocks();
                Strong<Item<45>> item   = make_managed_shared<Item<45>>( 1 );
                Weak<Item<45>>   weak   = item;
                g_allocator_storage.cleanup();
                item.reset();
                weak.reset();
                std::exit( pool_test_access::live_control_blocks() == before ? 0 : 1 ); // block back in the pool
            },
            testing::ExitedWithCode( 0 ), "" );
}

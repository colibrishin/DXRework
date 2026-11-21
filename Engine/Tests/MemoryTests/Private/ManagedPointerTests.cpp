#include <gtest/gtest.h>

#include "PoolTestAccess.h"

using Engine::Strong;
using Engine::Weak;

namespace pointer_tests
{
    inline int derived_destroyed = 0;
    struct Base { int a = 1; ~Base() = default; }; // non-virtual on purpose
    struct Derived : Base { int b = 2; ~Derived() { ++derived_destroyed; } };

    // Locks a weak pointer to itself while it is being destroyed.
    struct LocksSelfWhenDying
    {
        inline static bool  saw_bool = true;
        inline static void* saw_get  = reinterpret_cast<void*>( 1 );
        Weak<LocksSelfWhenDying> self;

        ~LocksSelfWhenDying()
        {
            const Strong<LocksSelfWhenDying> locked = self.lock();
            saw_bool = static_cast<bool>( locked );
            saw_get  = locked.get();
        }
    };

    const auto fast_failed = testing::ExitedWithCode( static_cast<int>( 0xC0000409u ) );
}
using namespace pointer_tests;

TEST( ManagedPointer, CopiesShareOneControlBlock )
{
    const Strong<Item<10>> a = make_managed_shared<Item<10>>( 1 );
    const Strong<Item<10>> b = a;
    const Strong<Item<10>> c = Weak<Item<10>>( a ).lock();

    EXPECT_TRUE( a.owner_equals( b ) );
    EXPECT_TRUE( a.owner_equals( c ) );
    EXPECT_EQ( a.use_count(), 3 );
    EXPECT_EQ( c.get(), a.get() );
}

TEST( ManagedPointer, LastOwnerDestroysTheObject )
{
    derived_destroyed = 0;
    Strong<Derived> a = make_managed_shared<Derived>();
    Strong<Derived> b = a;
    a.reset();
    EXPECT_EQ( derived_destroyed, 0 );
    b.reset();
    EXPECT_EQ( derived_destroyed, 1 );
}

TEST( ManagedPointer, HeldAsBaseDestroysTheAllocatedType )
{
    derived_destroyed = 0;
    Strong<Base> base = make_managed_shared<Derived, Base>();
    ASSERT_TRUE( base );
    EXPECT_EQ( base->a, 1 );
    base.reset();
    EXPECT_EQ( derived_destroyed, 1 );
}

TEST( ManagedPointer, NullAndEmptyAgree )
{
    const Strong<Item<11>> empty;
    EXPECT_FALSE( empty );
    EXPECT_TRUE( empty == nullptr );
    EXPECT_EQ( empty.get(), nullptr );
}

TEST( ManagedPointer, WeakLockedWhileObjectIsDyingIsEmpty )
{
    Strong<LocksSelfWhenDying> item = make_managed_shared<LocksSelfWhenDying>();
    item->self = item;
    item.reset();

    EXPECT_FALSE( LocksSelfWhenDying::saw_bool );
    EXPECT_EQ( LocksSelfWhenDying::saw_get, nullptr );
}

TEST( PoolAllocator, ObjectOutsideThePoolGetsNullContext )
{
    using T = Item<12>;
    (void)make_managed_shared<T>();
    T heap{};
    EXPECT_FALSE( g_allocator_storage.get_allocator<T>().get_context( &heap ).valid() );
}

TEST( PoolAllocator, UnmanagedRoundTrip )
{
    using T = Item<13>;
    auto& alloc = g_allocator_storage.get_allocator<T>();
    T*    item  = alloc.allocate_unmanaged( 5 );
    ASSERT_NE( item, nullptr );
    EXPECT_EQ( item->value, 5 );
    alloc.deallocate_unmanaged( item );
}

// ---- Misuse ends the process (__fastfail, 0xC0000409) instead of continuing with a wrong object. ----

TEST( ManagedPointerDeathTest, DereferenceAfterObjectWasFreedBehindIt )
{
    EXPECT_EXIT(
            {
                const Strong<Item<20>> item = make_managed_shared<Item<20>>( 1 );
                pool_test_access::release_behind( pool_test_access::core<Item<20>>(), item.get() );
                volatile int value = item->value;
                (void)value;
                std::exit( 0 );
            },
            fast_failed, "" );
}

TEST( PoolAllocatorDeathTest, ManagedPoolUsedUnmanaged )
{
    EXPECT_EXIT(
            {
                const Strong<Item<21>> item = make_managed_shared<Item<21>>( 1 );
                (void)g_allocator_storage.get_allocator<Item<21>>().allocate_unmanaged( 1 );
                std::exit( 0 );
            },
            fast_failed, "" );
}

TEST( PoolAllocatorDeathTest, DeallocateUnmanagedOfForeignObject )
{
    EXPECT_EXIT(
            {
                auto& alloc = g_allocator_storage.get_allocator<Item<22>>();
                alloc.deallocate_unmanaged( alloc.allocate_unmanaged( 1 ) ); // pool is unmanaged
                Item<22> foreign{};
                alloc.deallocate_unmanaged( &foreign );
                std::exit( 0 );
            },
            fast_failed, "" );
}

TEST( PoolAllocatorDeathTest, DeallocateUnmanagedOfInteriorPointer )
{
    EXPECT_EXIT(
            {
                auto&     alloc = g_allocator_storage.get_allocator<PairItem>();
                PairItem* item  = alloc.allocate_unmanaged();
                alloc.deallocate_unmanaged( reinterpret_cast<PairItem*>( &item->b ) );
                std::exit( 0 );
            },
            fast_failed, "" );
}

TEST( PoolAllocatorDeathTest, DeallocateUnmanagedTwice )
{
    EXPECT_EXIT(
            {
                auto& alloc = g_allocator_storage.get_allocator<Item<23>>();
                Item<23>* item = alloc.allocate_unmanaged( 1 );
                alloc.deallocate_unmanaged( item );
                alloc.deallocate_unmanaged( item );
                std::exit( 0 );
            },
            fast_failed, "" );
}

// ---- After cleanup (process shutdown) every managed pointer reads as null. Cleanup cannot be undone, so it runs
// in a child process; the exit code carries the result. ----

TEST( ManagedPointerDeathTest, AfterCleanupEveryAccessReadsNull )
{
    EXPECT_EXIT(
            {
                const Strong<Item<30>> item = make_managed_shared<Item<30>>( 1 );
                g_allocator_storage.cleanup();
                const bool ok = item.get() == nullptr && item == nullptr && !item;
                std::exit( ok ? 0 : 1 );
            },
            testing::ExitedWithCode( 0 ), "" );
}

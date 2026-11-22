#include <gtest/gtest.h>

#include "PoolTestAccess.h"

#include <type_traits>
#include <utility>
#include <vector>

using Engine::Strong;
using Engine::Weak;

// Moving a managed pointer transfers its share (no reference count change) and leaves the source empty, so
// std::vector relocates elements without copying them.

namespace move_tests
{
    inline int derived_destroyed = 0;
    struct Base { int a = 1; };
    struct Derived : Base { int b = 2; ~Derived() { ++derived_destroyed; } };
}
using namespace move_tests;

TEST( ManagedPointerMove, MovesAreNoexcept )
{
    EXPECT_TRUE( std::is_nothrow_move_constructible_v<Strong<Item<60>>> );
    EXPECT_TRUE( std::is_nothrow_move_assignable_v<Strong<Item<60>>> );
    EXPECT_TRUE( ( std::is_nothrow_constructible_v<Strong<Base>, Strong<Derived>&&> ) );
    EXPECT_TRUE( std::is_nothrow_move_constructible_v<Weak<Item<60>>> );
    EXPECT_TRUE( std::is_nothrow_move_assignable_v<Weak<Item<60>>> );
}

TEST( ManagedPointerMove, MoveConstructTransfersTheShare )
{
    Strong<Item<61>> a      = make_managed_shared<Item<61>>( 7 );
    const void*      object = a.get();

    const Strong<Item<61>> b( std::move( a ) );

    EXPECT_EQ( b.get(), object );
    EXPECT_EQ( b.use_count(), 1 );
    EXPECT_EQ( b->value, 7 );
    EXPECT_FALSE( a ); // NOLINT(bugprone-use-after-move): the moved-from state is what is tested
    EXPECT_TRUE( a == nullptr );
    EXPECT_EQ( a.get(), nullptr );
}

TEST( ManagedPointerMove, MoveAssignReleasesTheOldObject )
{
    derived_destroyed = 0;
    Strong<Derived> target = make_managed_shared<Derived>();
    Strong<Derived> source = make_managed_shared<Derived>();
    const void*     object = source.get();

    target = std::move( source );

    EXPECT_EQ( derived_destroyed, 1 ); // the object target held
    EXPECT_EQ( target.get(), object );
    EXPECT_EQ( target.use_count(), 1 );
    EXPECT_FALSE( source ); // NOLINT(bugprone-use-after-move)
    target.reset();
    EXPECT_EQ( derived_destroyed, 2 );
}

// Guard (also held before moves existed).
TEST( ManagedPointerMove, SelfMoveAssignKeepsTheObject )
{
    Strong<Item<62>> a      = make_managed_shared<Item<62>>( 3 );
    Strong<Item<62>>& alias = a;
    a                       = std::move( alias );
    ASSERT_TRUE( a );
    EXPECT_EQ( a->value, 3 );
    EXPECT_EQ( a.use_count(), 1 );
}

TEST( ManagedPointerMove, ConvertingMoveToBase )
{
    derived_destroyed = 0;
    Strong<Derived> derived = make_managed_shared<Derived>();
    const Base*     base_ptr = derived.get();

    Strong<Base> base( std::move( derived ) );

    EXPECT_EQ( base.get(), base_ptr );
    EXPECT_EQ( base.use_count(), 1 );
    EXPECT_FALSE( derived ); // NOLINT(bugprone-use-after-move)
    base.reset();
    EXPECT_EQ( derived_destroyed, 1 ); // still destroyed as the allocated type
}

// Guard (also held before moves existed).
TEST( ManagedPointerMove, ConvertingCopyToBaseSharesOneBlock )
{
    const Strong<Derived> derived = make_managed_shared<Derived>();
    const Strong<Base>    base    = derived;
    EXPECT_EQ( base.get(), static_cast<const Base*>( derived.get() ) );
    EXPECT_EQ( derived.use_count(), 2 );
    EXPECT_TRUE( base.owner_equals( derived ) );
}

// Guard (also held before moves existed).
TEST( ManagedPointerMove, MovedFromCanBeReused )
{
    Strong<Item<63>> a = make_managed_shared<Item<63>>( 1 );
    Strong<Item<63>> b( std::move( a ) );
    a = make_managed_shared<Item<63>>( 2 ); // NOLINT(bugprone-use-after-move)
    ASSERT_TRUE( a );
    EXPECT_EQ( a->value, 2 );
    EXPECT_EQ( b->value, 1 );
}

// Guard (also held before moves existed).
TEST( ManagedPointerMove, VectorGrowthKeepsEveryShare )
{
    std::vector<Strong<Item<64>>> items;
    for ( int i = 0; i < 1000; ++i ) { items.push_back( make_managed_shared<Item<64>>( i ) ); }
    for ( int i = 0; i < 1000; ++i )
    {
        ASSERT_TRUE( items[ i ] );
        EXPECT_EQ( items[ i ]->value, i );
        EXPECT_EQ( items[ i ].use_count(), 1 );
    }
}

TEST( ManagedPointerMove, MovedFromReadsAsNullEverywhere )
{
    Strong<Item<67>>       a = make_managed_shared<Item<67>>( 1 );
    const Strong<Item<67>> b( std::move( a ) );
    const Strong<Item<67>> null;

    EXPECT_EQ( std::hash<Strong<Item<67>>>{}( a ), std::hash<Strong<Item<67>>>{}( null ) ); // NOLINT(bugprone-use-after-move)
    EXPECT_TRUE( Weak<Item<67>>( a ).empty() );
    EXPECT_FALSE( a.owner_equals( b ) );
}

TEST( ManagedPointerMove, MoveAssignNullReleasesTheObject )
{
    derived_destroyed = 0;
    Strong<Derived> item = make_managed_shared<Derived>();
    Strong<Derived> null;
    item = std::move( null );
    EXPECT_EQ( derived_destroyed, 1 );
    EXPECT_FALSE( item );
}

namespace move_tests
{
    // Holds the next node; assigning node = std::move( node->next ) releases the node that owns the source.
    struct Node
    {
        inline static int destroyed = 0;
        int               value     = 0;
        Strong<Node>      next;
        Node() = default;
        explicit Node( const int v ) : value( v ) {}
        ~Node() { ++destroyed; }
    };
}

// Guard: the source lives inside the object the assignment releases. (The old assignment order also passes here,
// because the stale read hits still-mapped pool memory; this pins the observable result.)
TEST( ManagedPointerMove, MoveAssignFromInsideTheOldObject )
{
    Node::destroyed   = 0;
    Strong<Node> head = make_managed_shared<Node>( 1 );
    head->next        = make_managed_shared<Node>( 2 );

    head = std::move( head->next );

    EXPECT_EQ( Node::destroyed, 1 );
    ASSERT_TRUE( head );
    EXPECT_EQ( head->value, 2 );
    EXPECT_EQ( head.use_count(), 1 );
}

TEST( WeakPointer, ResetEmptiesIt )
{
    const Strong<Item<65>> item = make_managed_shared<Item<65>>( 1 );
    Weak<Item<65>>         weak = item;
    weak.reset();
    EXPECT_TRUE( weak.empty() );
    EXPECT_FALSE( weak.lock() );
    EXPECT_FALSE( weak.owner_equals( item ) );
}

TEST( WeakPointer, MoveTransfersAndEmptiesTheSource )
{
    const Strong<Item<66>> item = make_managed_shared<Item<66>>( 1 );
    Weak<Item<66>>         a    = item;

    Weak<Item<66>> b( std::move( a ) );
    EXPECT_TRUE( b.owner_equals( item ) );
    EXPECT_EQ( b.lock().get(), item.get() );
    EXPECT_TRUE( a.empty() ); // NOLINT(bugprone-use-after-move)
    EXPECT_FALSE( a.lock() );

    Weak<Item<66>> c;
    c = std::move( b );
    EXPECT_EQ( c.lock().get(), item.get() );
    EXPECT_TRUE( b.empty() ); // NOLINT(bugprone-use-after-move)
}

// managed_static_weak_cast<T>(Strong<U>): a Weak<T> without a temporary Strong<T> (GetComponent<T>()).
namespace weak_cast_tests
{
    struct A { int a = 1; };
    struct B { int b = 2; };
    struct C : A, B { int c = 3; }; // B sits at a non-zero offset inside C
}

TEST( ManagedWeakCast, UpcastAdjustsThePointerAndDoesNotOwn )
{
    using namespace weak_cast_tests;
    Strong<C>     c  = make_managed_shared<C>();
    const Weak<B> wb = managed_static_weak_cast<B>( c );

    EXPECT_EQ( c.use_count(), 1 ); // the weak pointer holds no share
    const Strong<B> b = wb.lock();
    ASSERT_TRUE( b );
    EXPECT_EQ( b.get(), static_cast<B*>( c.get() ) );
    EXPECT_EQ( b->b, 2 );
    EXPECT_TRUE( wb.owner_equals( c ) );
}

TEST( ManagedWeakCast, DowncastReachesTheDerivedObject )
{
    using namespace weak_cast_tests;
    const Strong<C> c = make_managed_shared<C>();
    const Strong<B> b = c;
    const Weak<C>   wc = managed_static_weak_cast<C>( b );

    const Strong<C> locked = wc.lock();
    ASSERT_TRUE( locked );
    EXPECT_EQ( locked.get(), c.get() );
    EXPECT_EQ( locked->c, 3 );
}

TEST( ManagedWeakCast, SameOwnerAsAConvertedWeak )
{
    using namespace weak_cast_tests;
    const Strong<C> c    = make_managed_shared<C>();
    const Weak<B>   cast = managed_static_weak_cast<B>( c );
    const Weak<B>   conv = Weak<B>( Strong<B>( c ) );

    EXPECT_TRUE( cast == conv );
    EXPECT_EQ( std::hash<Weak<B>>{}( cast ), std::hash<Weak<B>>{}( conv ) );
    EXPECT_EQ( cast.lock().get(), conv.lock().get() );
}

TEST( ManagedWeakCast, LocksThroughAnotherOwnerAfterTheSourceIsGone )
{
    using namespace weak_cast_tests;
    Strong<C>       source = make_managed_shared<C>();
    const Strong<C> keep   = source;
    const Weak<B>   wb     = managed_static_weak_cast<B>( source );

    source.reset();
    const Strong<B> b = wb.lock();
    ASSERT_TRUE( b );
    EXPECT_EQ( b.get(), static_cast<B*>( keep.get() ) );
    EXPECT_EQ( b->b, 2 );
}

TEST( ManagedWeakCast, ExpiresWithTheObject )
{
    using namespace weak_cast_tests;
    Strong<C>     c  = make_managed_shared<C>();
    const Weak<A> wa = managed_static_weak_cast<A>( c );

    c.reset();
    EXPECT_TRUE( wa.expired() );
    EXPECT_FALSE( wa.lock() );
}

TEST( ManagedWeakCast, EmptySourceGivesAnEmptyWeak )
{
    using namespace weak_cast_tests;
    const Strong<C> empty;
    const Weak<B>   wb = managed_static_weak_cast<B>( empty );

    EXPECT_TRUE( wb.empty() );
    EXPECT_FALSE( wb.lock() );
}

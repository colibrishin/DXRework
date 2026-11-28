#include <gtest/gtest.h>

#include "Delegation.hpp"
#include "TestListener.h"

// Delegate behaviour through the API that exists before and after the Delegate rework (Listen/Remove with a weak
// owner, Broadcast, copy). They must hold on both.

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
}

TEST( DelegateShared, CallsEachListener )
{
    Delegate<int> delegate;
    const auto    a = MakeListener();
    const auto    b = MakeListener();
    delegate.Listen( WeakOf( a ), &TestListener::OnSignal );
    delegate.Listen( WeakOf( b ), &TestListener::OnSignal );

    delegate.Broadcast( 3 );

    EXPECT_EQ( a->sum, 3 );
    EXPECT_EQ( b->sum, 3 );
}

TEST( DelegateShared, ListenersRemovingThemselvesDuringBroadcast )
{
    Delegate<int> delegate;
    const auto    a = MakeListener();
    const auto    b = MakeListener();
    delegate.Listen( WeakOf( a ), &TestListener::OnSignal );
    delegate.Listen( WeakOf( b ), &TestListener::OnSignal );
    a->action = [ & ] { delegate.Remove( WeakOf( a ), &TestListener::OnSignal ); };
    b->action = [ & ] { delegate.Remove( WeakOf( b ), &TestListener::OnSignal ); };

    delegate.Broadcast( 1 );
    EXPECT_EQ( a->calls, 1 );
    EXPECT_EQ( b->calls, 1 );

    delegate.Broadcast( 1 );
    EXPECT_EQ( a->calls, 1 );
    EXPECT_EQ( b->calls, 1 );
}

TEST( DelegateShared, NestedBroadcastWithARemoval )
{
    // Whichever listener runs first, a is called by the outer and the nested broadcast, and b exactly once: it
    // removes itself, so the broadcast that has not reached it yet skips it.
    Delegate<int> delegate;
    const auto    a = MakeListener();
    const auto    b = MakeListener();
    delegate.Listen( WeakOf( a ), &TestListener::OnSignal );
    delegate.Listen( WeakOf( b ), &TestListener::OnSignal );
    a->action = [ & ]
    {
        if ( a->calls == 1 )
        {
            delegate.Broadcast( 10 );
        }
    };
    b->action = [ & ] { delegate.Remove( WeakOf( b ), &TestListener::OnSignal ); };

    delegate.Broadcast( 1 );

    EXPECT_EQ( a->calls, 2 );
    EXPECT_EQ( b->calls, 1 );
}

TEST( DelegateShared, CopyDoesNotCallTheOriginalsListeners )
{
    Delegate<int> delegate;
    const auto    a = MakeListener();
    delegate.Listen( WeakOf( a ), &TestListener::OnSignal );

    Delegate<int> copy = delegate;
    copy.Broadcast( 1 );

    EXPECT_EQ( a->calls, 0 );
}

TEST( DelegateShared, ExpiredListenerIsNotCalled )
{
    Delegate<int> delegate;
    auto          a = MakeListener();
    const auto    b = MakeListener();
    delegate.Listen( WeakOf( a ), &TestListener::OnSignal );
    delegate.Listen( WeakOf( b ), &TestListener::OnSignal );
    a.reset();

    delegate.Broadcast( 1 );

    EXPECT_EQ( b->calls, 1 );
}

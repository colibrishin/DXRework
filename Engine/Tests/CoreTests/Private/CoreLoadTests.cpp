#include <gtest/gtest.h>

#include "Delegation.hpp"
#include "Transform.h"

// The test executable links Core.dll and can create Core entities without the engine running.

TEST( CoreLoad, CreatesAnEntityAndWeakensIt )
{
    const Engine::Strong<Engine::Components::Transform> transform =
            make_managed_shared<Engine::Components::Transform>( Engine::Weak<Engine::Abstracts::ObjectBase>{} );
    ASSERT_TRUE( transform );

    const Engine::Weak<Engine::Components::Transform> weak = transform;
    EXPECT_TRUE( weak.lock() );
}

TEST( CoreLoad, BroadcastsToAFunctionListener )
{
    static int calls = 0;
    calls = 0;
    Delegate<int> delegate;
    delegate.Listen( []( int value ) { calls += value; } );
    delegate.Broadcast( 2 );
    EXPECT_EQ( calls, 2 );
}

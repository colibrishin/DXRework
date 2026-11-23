#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <windows.h>

#include "Delegation.hpp"

// A listener registered by a module that is then unloaded (CoreTestsPlugin.dll, next to this executable).

namespace
{
    using Register   = void ( * )( ConcurrentDelegate<int>* );
    using Unregister = void ( * )();
    using Calls      = int ( * )();

    struct Plugin
    {
        HMODULE    module     = LoadLibraryW( L"CoreTestsPlugin.dll" );
        Register   reg        = reinterpret_cast<Register>( GetProcAddress( module, "PluginRegister" ) );
        Register   reg_leaked = reinterpret_cast<Register>( GetProcAddress( module, "PluginRegisterLeaked" ) );
        Unregister unreg      = reinterpret_cast<Unregister>( GetProcAddress( module, "PluginUnregister" ) );
        Calls      calls      = reinterpret_cast<Calls>( GetProcAddress( module, "PluginCalls" ) );
    };
}

TEST( DelegatePlugin, UnloadAfterRemovingWhileOtherThreadsBroadcast )
{
    ConcurrentDelegate<int> delegate;
    Plugin                  plugin;
    ASSERT_TRUE( plugin.module && plugin.reg && plugin.unreg && plugin.calls );

    plugin.reg( &delegate );

    std::atomic<bool>        stop = false;
    std::vector<std::thread> broadcasters;
    for ( int i = 0; i < 4; ++i )
    {
        broadcasters.emplace_back( [ & ]
        {
            while ( !stop )
            {
                delegate.Broadcast( 1 );
            }
        } );
    }

    while ( plugin.calls() == 0 )
    {
        std::this_thread::yield();
    }
    plugin.unreg();
    ASSERT_TRUE( FreeLibrary( plugin.module ) );

    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    stop = true;
    for ( std::thread& broadcaster : broadcasters )
    {
        broadcaster.join();
    }

    EXPECT_EQ( delegate.ListenerCount(), 0u );
    EXPECT_EQ( GetModuleHandleW( L"CoreTestsPlugin.dll" ), nullptr );
}

// The failure the rule above prevents: a registration left behind by an unloaded module crashes the next broadcast.
TEST( DelegatePluginDeathTest, RegistrationLeftByAnUnloadedModuleCrashesTheBroadcast )
{
    EXPECT_DEATH(
            {
                ConcurrentDelegate<int> delegate;
                Plugin                  plugin;
                plugin.reg_leaked( &delegate );
                FreeLibrary( plugin.module );
                delegate.Broadcast( 1 );
            },
            "" );
}

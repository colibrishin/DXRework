#include <atomic>

#include "Delegation.hpp"

// Registers a listener whose code lives in this DLL. CoreTests unloads the DLL afterwards.

namespace
{
    Engine::DelegateHandle g_handle;
    std::atomic<int>       g_calls = 0;
}

extern "C" __declspec( dllexport ) void PluginRegister( ConcurrentDelegate<int>* delegate )
{
    g_handle = delegate->ListenScoped( []( const int value ) { g_calls += value; } );
}

extern "C" __declspec( dllexport ) void PluginUnregister()
{
    g_handle.Reset();
}

// The registration outlives the DLL: nothing removes it before FreeLibrary.
extern "C" __declspec( dllexport ) void PluginRegisterLeaked( ConcurrentDelegate<int>* delegate )
{
    new Engine::DelegateHandle( delegate->ListenScoped( []( const int value ) { g_calls += value; } ) );
}

extern "C" __declspec( dllexport ) int PluginCalls()
{
    return g_calls;
}

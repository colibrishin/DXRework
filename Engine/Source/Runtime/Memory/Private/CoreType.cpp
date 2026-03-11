#include "CoreType.h"

PoolAllocatorStorage g_allocator_storage{};

void PoolAllocatorStorage::cleanup()
{
    for ( const pool_allocator_base* allocator : m_allocators_ | std::views::values )
    {
        allocator->destroy();
    }

    for ( const pool_allocator_base* allocator : m_allocators_ | std::views::values )
    {
        allocator->purge();
    }

    m_allocators_.clear();
}

void PoolAllocatorStorage::report_leakage()
{
#if _WIN32 || _WIN64
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#if !defined( ENGINE_SUPPRESS_POOL_LEAK_REPORT )
    // Boost pool allocators use static/singleton storage; blocks still reachable at exit
    // are reported as leaks but are intentional. Define ENGINE_SUPPRESS_POOL_LEAK_REPORT
    // to skip the dump when you only expect pool-related "leaks".
    _CrtDumpMemoryLeaks();
#endif
#endif
}

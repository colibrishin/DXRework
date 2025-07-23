#include "CoreType.h"

PoolAllocatorStorage g_allocator_storage{};

void PoolAllocatorStorage::cleanup()
{
    for ( const pool_allocator_base* allocator : m_allocators_ | std::views::values )
    {
        allocator->destroy();
    }

    m_allocators_.clear();
}

inline void PoolAllocatorStorage::report_leakage()
{
#if _WIN32 || _WIN64
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    _CrtDumpMemoryLeaks();
#endif
}

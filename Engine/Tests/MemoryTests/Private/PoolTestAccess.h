#pragma once

#include "CoreType.h"

#include <cstdlib>

// Pooled test types. Each test uses its own N, so pools start empty and their ownership (managed/unmanaged) is not
// shared between tests.
template <int N>
struct Item
{
    int value = 0;
    Item() = default;
    explicit Item( const int v ) : value( v ) {}
};

struct PairItem
{
    int a = 0;
    int b = 0;
};

// Test-only access to pool internals (friend of pool_core and object_pool_allocator).
struct pool_test_access
{
    template <typename T>
    static pool_core* core()
    {
        return g_allocator_storage.get_allocator<T>().m_core_;
    }

    static const void* start( const pool_core* c ) { return c->m_start_ptr_; }
    static const void* mask( const pool_core* c ) { return c->m_mask_; }
    static const void* generations( const pool_core* c ) { return c->m_local_allocation_count_; }
    static size_t      committed_chunks( const pool_core* c ) { return c->m_chunks_.load(); }

    // Bumps the slot's generation and marks it free without destroying the object: simulates an object freed
    // behind a managed pointer.
    static void release_behind( pool_core* c, const void* object ) { c->release( c->key_of( object ) ); }
};

#include "CoreType.h"

PoolAllocatorStorage g_allocator_storage{};

namespace
{
    constexpr size_t slab_slots = 1 << 8; // slots per mask entry (one __m256i)

    boost::pool<user_allocator>& pool_of( void* pool )
    {
        return *static_cast<boost::pool<user_allocator>*>( pool );
    }
}

pool_core::pool_core( const size_t element_size )
    : m_element_size_( element_size ),
      // Per-instance pool, not boost::singleton_pool: a singleton is a static in every DLL that instantiates it.
      m_pool_( new boost::pool<user_allocator>( element_size ) )
{ }

void pool_core::lock() const
{
    while ( m_lock_.exchange( true ) )
    { }
}

void pool_core::unlock() const
{
    m_lock_.store( false );
}

void pool_core::init()
{
    if ( !m_initialized_ )
    {
        m_initialized_ = true;
        reallocate();
    }
}

void pool_core::reallocate()
{
    auto& pool        = pool_of( m_pool_ );
    char* reallocated = static_cast<char*>( pool.ordered_malloc( ( m_mask_.size() + 1 ) * slab_slots ) );
    if ( m_start_ptr_ )
    {
        std::memmove( reallocated, m_start_ptr_, m_element_size_ * m_mask_.size() * slab_slots );
        pool.ordered_free( m_start_ptr_, m_mask_.size() * slab_slots );
    }
    m_mask_.emplace_back( __m256i{} );
    m_local_allocation_count_.resize( ( m_mask_.size() + 1 ) * slab_slots, 0 );
    ++m_reallocation_count_;
    m_start_ptr_ = reallocated;
}

AllocationKey pool_core::get_free_space()
{
    auto it = m_mask_.begin();
    while ( it != m_mask_.end() )
    {
        if ( _mm256_test_all_ones( *it ) )
        {
            ++it;
            continue;
        }

        break;
    }

    const size_t chunk = std::distance( m_mask_.begin(), it );

    if ( it == m_mask_.end() )
    {
        reallocate();
        return { ( size_t )m_local_allocation_count_[ chunk * slab_slots ], ( size_t )chunk, ( uint8_t )0, ( uint8_t )0 };
    }

    for ( uint8_t segment = 0; segment < 8; ++segment )
    {
        const uint32_t& segment_value = it->m256i_u32[ segment ];
        if ( segment_value == std::numeric_limits<uint32_t>::max() )
        {
            continue;
        }

        const uint8_t offset = ( uint8_t )_tzcnt_u32( ~( ( *it ).m256i_i32[ segment ] ) );
        const size_t  unwrapped_index =
                ( chunk * slab_slots ) + ( segment * std::numeric_limits<uint32_t>::digits ) + offset;
        return { ( size_t )m_local_allocation_count_[ unwrapped_index ], chunk, segment, offset };
    }

    return { ( size_t )-1, ( size_t )-1, ( uint8_t )-1, ( uint8_t )-1 };
}

bool pool_core::peak( const AllocationKey& key ) const
{
    const uint32_t shift            = m_mask_[ key.chunk ].m256i_i32[ key.segment ] >> key.offset;
    const size_t   allocation_count = m_local_allocation_count_[ key.to_raw_index() ];
    return ( bool )( shift & 1 ) && key.allocation_count == allocation_count;
}

void pool_core::flip( const AllocationKey& key, const bool allocate )
{
    if ( allocate )
    {
        m_mask_[ key.chunk ].m256i_i32[ key.segment ] |= 1 << key.offset;
    }
    else
    {
        m_mask_[ key.chunk ].m256i_i32[ key.segment ] &= ~( 1 << key.offset );
    }
}

AllocationContext pool_core::reserve( const HashType type )
{
    init();
    lock();
    const AllocationKey     key = get_free_space();
    const AllocationContext context( key, this, type );
    flip( key, true );
    unlock();
    return context;
}

bool pool_core::alive( const AllocationKey& key ) const
{
    lock();
    const bool result = peak( key );
    unlock();
    return result;
}

void pool_core::release( const AllocationKey& key )
{
    lock();
    ++m_local_allocation_count_[ key.to_raw_index() ];
    flip( key, false );
    unlock();
}

void* pool_core::slot_ptr( const AllocationKey& key ) const
{
    return m_start_ptr_ + key.to_raw_index() * m_element_size_;
}

AllocationKey pool_core::key_of( const void* ptr ) const
{
    const size_t dist    = ( static_cast<const char*>( ptr ) - m_start_ptr_ ) / m_element_size_;
    const size_t chunk   = dist / slab_slots;
    const size_t segment = ( dist - ( slab_slots * chunk ) ) / std::numeric_limits<uint32_t>::digits;
    const size_t offset  = ( dist - ( slab_slots * chunk ) ) - ( segment * std::numeric_limits<uint32_t>::digits );
    return { m_local_allocation_count_[ dist ], chunk, ( uint8_t )segment, ( uint8_t )offset };
}

bool pool_core::is_safe( const void* ptr ) const
{
    const char* p = static_cast<const char*>( ptr );
    return m_start_ptr_ <= p && m_start_ptr_ + ( m_mask_.size() * slab_slots * m_element_size_ ) > p;
}

bool pool_core::past( const AllocationContext& context ) const
{
    return context.type_allocation_count() != m_reallocation_count_ ||
           context.allocation_count() != m_local_allocation_count_[ context.m_key_.to_raw_index() ];
}

bool pool_core::expired( const AllocationContext& context ) const
{
    return !peak( context.m_key_ );
}

void* pool_core::get_ptr( const AllocationContext& context ) const
{
    if ( !context.valid() )
    {
        return nullptr;
    }

    if ( context.m_key_.allocation_count != m_local_allocation_count_[ context.m_key_.to_raw_index() ] )
    {
        return nullptr;
    }

    void* ptr = m_start_ptr_ + context.m_key_.to_raw_index() * m_element_size_;
    if ( is_safe( ptr ) )
    {
        return ptr;
    }

    return nullptr;
}

void pool_core::destroy() const
{
    pool_of( m_pool_ ).ordered_free( m_start_ptr_, m_mask_.size() * slab_slots );
}

void pool_core::purge() const
{
    pool_of( m_pool_ ).release_memory();
    pool_of( m_pool_ ).purge_memory();
}

pool_core* PoolAllocatorStorage::get_core( const HashType type, const size_t element_size )
{
    std::lock_guard<std::mutex> lock( m_allocators_mutex_ );
    auto                        it = m_allocators_.find( type );
    if ( it == m_allocators_.end() )
    {
        // Never deleted: contexts in any module may refer to it until the process exits.
        it = m_allocators_.emplace( type, new pool_core( element_size ) ).first;
    }

    assert( it->second->element_size() == element_size );
    return it->second;
}

void PoolAllocatorStorage::cleanup()
{
    std::lock_guard<std::mutex> lock( m_allocators_mutex_ );
    m_closed_.store( true, std::memory_order_release );

    for ( const pool_core* allocator : m_allocators_ | std::views::values )
    {
        allocator->destroy();
    }

    for ( const pool_core* allocator : m_allocators_ | std::views::values )
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

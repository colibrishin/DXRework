#include "CoreType.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <new>
#include <stdexcept>
#include <string>

PoolAllocatorStorage g_allocator_storage{};

namespace
{
    constexpr size_t slab_slots = 1 << 8; // slots per mask entry (one __m256i)

    char* reserve_range( const size_t bytes )
    {
        void* p = VirtualAlloc( nullptr, bytes, MEM_RESERVE, PAGE_NOACCESS );
        if ( !p )
        {
            throw std::bad_alloc();
        }
        return static_cast<char*>( p );
    }

    void commit_range( char* begin, const size_t bytes )
    {
        // Pages that are already committed (shared with the previous chunk) are left as they are.
        if ( !VirtualAlloc( begin, bytes, MEM_COMMIT, PAGE_READWRITE ) )
        {
            throw std::bad_alloc();
        }
    }

    struct spin_guard
    {
        std::atomic<bool>& flag;
        explicit spin_guard( std::atomic<bool>& f ) : flag( f )
        {
            while ( flag.exchange( true ) )
            { }
        }
        ~spin_guard() { flag.store( false ); }
    };
}

pool_core::pool_core( const size_t element_size, const size_t reserve_bytes )
    : m_element_size_( element_size ),
      m_reserve_bytes_( reserve_bytes )
{ }

// Called under the lock.
void pool_core::init()
{
    if ( m_initialized_ )
    {
        return;
    }

    const size_t capacity_chunks = ( std::max )( m_reserve_bytes_ / ( m_element_size_ * slab_slots ), size_t( 1 ) );
    const size_t slots           = capacity_chunks * slab_slots;

    // Reserve all three ranges before keeping any, so a failure does not leak the ranges reserved so far.
    char* objects     = nullptr;
    char* mask        = nullptr;
    char* generations = nullptr;
    try
    {
        objects     = reserve_range( slots * m_element_size_ );
        mask        = reserve_range( capacity_chunks * sizeof( __m256i ) );
        generations = reserve_range( slots * sizeof( size_t ) );
    }
    catch ( ... )
    {
        for ( char* range : { objects, mask, generations } )
        {
            if ( range )
            {
                VirtualFree( range, 0, MEM_RELEASE );
            }
        }
        throw;
    }

    m_capacity_chunks_        = capacity_chunks;
    m_start_ptr_              = objects;
    m_mask_                   = reinterpret_cast<__m256i*>( mask );
    m_local_allocation_count_ = reinterpret_cast<size_t*>( generations );
    m_initialized_            = true;
    commit_chunk();
}

// Called under the lock. Commits the next 256 slots, their mask entry and their generations; committed pages are
// zero-filled, so the new slots are free with generation 0. Nothing that was handed out moves.
void pool_core::commit_chunk()
{
    const size_t chunk = m_chunks_.load( std::memory_order_relaxed );
    if ( chunk >= m_capacity_chunks_ )
    {
        throw std::length_error( "object pool: reserved address space exhausted (element size " +
                                 std::to_string( m_element_size_ ) + " bytes, " + std::to_string( m_reserve_bytes_ ) +
                                 " bytes reserved; specialize pool_reserve_bytes for the type)" );
    }

    commit_range( m_start_ptr_ + chunk * slab_slots * m_element_size_, slab_slots * m_element_size_ );
    commit_range( reinterpret_cast<char*>( m_mask_ + chunk ), sizeof( __m256i ) );
    commit_range( reinterpret_cast<char*>( m_local_allocation_count_ + chunk * slab_slots ), slab_slots * sizeof( size_t ) );
    m_chunks_.store( chunk + 1, std::memory_order_release );
}

AllocationKey pool_core::get_free_space()
{
    const size_t chunks = m_chunks_.load( std::memory_order_relaxed );
    size_t       chunk  = 0;
    while ( chunk < chunks && _mm256_test_all_ones( m_mask_[ chunk ] ) )
    {
        ++chunk;
    }

    if ( chunk == chunks )
    {
        commit_chunk();
        return { ( size_t )m_local_allocation_count_[ chunk * slab_slots ], ( size_t )chunk, ( uint8_t )0, ( uint8_t )0 };
    }

    const __m256i& entry = m_mask_[ chunk ];
    for ( uint8_t segment = 0; segment < 8; ++segment )
    {
        const uint32_t& segment_value = entry.m256i_u32[ segment ];
        if ( segment_value == std::numeric_limits<uint32_t>::max() )
        {
            continue;
        }

        const uint8_t offset = ( uint8_t )_tzcnt_u32( ~( entry.m256i_i32[ segment ] ) );
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

void pool_core::claim_ownership( const ownership mode )
{
    ownership current = m_ownership_.load( std::memory_order_relaxed );
    if ( current == mode )
    {
        return;
    }

    if ( current == ownership::undecided &&
         ( m_ownership_.compare_exchange_strong( current, mode ) || current == mode ) )
    {
        return;
    }

    fail_fast( "object pool: a type is allocated both with and without managed pointers" );
}

void pool_core::fail_fast( const char* reason )
{
    OutputDebugStringA( reason );
    OutputDebugStringA( "\n" );
    __fastfail( FAST_FAIL_FATAL_APP_EXIT );
}

AllocationContext pool_core::reserve( const HashType type )
{
    // The lock is released by the guard if the reservation or a commit throws.
    spin_guard              guard( m_lock_ );
    init();
    const AllocationKey     key = get_free_space();
    const AllocationContext context( key, this, type );
    flip( key, true );
    return context;
}

bool pool_core::alive( const AllocationKey& key ) const
{
    spin_guard guard( m_lock_ );
    return peak( key );
}

void pool_core::release( const AllocationKey& key )
{
    spin_guard guard( m_lock_ );
    ++m_local_allocation_count_[ key.to_raw_index() ];
    flip( key, false );
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
    return m_start_ptr_ && m_start_ptr_ <= p &&
           m_start_ptr_ + ( m_chunks_.load( std::memory_order_acquire ) * slab_slots * m_element_size_ ) > p;
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

// The storage is kept until the process exits: managed pointers released after cleanup (static destructors,
// modules unloaded later) still read the slot mask and generations.
void pool_core::destroy() const
{ }

void pool_core::purge() const
{ }

pool_core* PoolAllocatorStorage::get_core( const HashType type, const size_t element_size, const size_t reserve_bytes )
{
    std::lock_guard<std::mutex> lock( m_allocators_mutex_ );
    auto                        it = m_allocators_.find( type );
    if ( it == m_allocators_.end() )
    {
        // Never deleted: contexts in any module may refer to it until the process exits.
        it = m_allocators_.emplace( type, new pool_core( element_size, reserve_bytes ) ).first;
    }

    assert( it->second->element_size() == element_size );
    assert( it->second->reserve_bytes() == reserve_bytes && "pool_reserve_bytes<T> differs between modules" );
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
    // Object pools reserve their storage with VirtualAlloc and are not part of this report. Container pools (boost)
    // still keep blocks until exit, which are reported as leaks but are intentional. Define
    // ENGINE_SUPPRESS_POOL_LEAK_REPORT to skip the dump when you only expect pool-related "leaks".
    _CrtDumpMemoryLeaks();
#endif
#endif
}

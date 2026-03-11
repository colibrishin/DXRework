#pragma once
#include <set>
#include <tuple>
#include <unordered_map>
#include <map>
#include <queue>
#include <vector>
#include "CoreType.h"

#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/align/aligned_allocator.hpp>

#if defined(__GNUC__) || defined(__clang__)
#  define ALIGN(x) __attribute__ ((aligned(x)))
#elif defined(_MSC_VER)
#  define ALIGN(x) __declspec(align(x))
#else
#  error "Unknown compiler; can't define ALIGN"
#endif

#if defined(__GNUC__) || defined(__clang__)
#    define ALIGNOF(X) __alignof__(X)
#elif defined(_MSC_VER)
#    define ALIGNOF(X) __alignof(X)
#else
#  error "Unknown compiler; can't define ALIGNOF"
#endif

namespace Engine
{
    constexpr uint64_t Align( uint64_t size, uint64_t alignment )
    {
        return ( size + alignment - 1 ) & ~( alignment - 1 );
    }

    consteval size_t nearest_pow_two( size_t value )
    {
        size_t result = 1;

        while ( value > result )
        {
            result = result << 1;
        }

        return result;
    }
}

namespace Engine
{
    struct alloc_base;
}

extern ENGINE_MEMORY_API std::unordered_map<size_t, Engine::alloc_base*> g_static_alloc;

namespace Engine
{
    // RTTI-free type key: unique address per type for hashing and map keys.
    template <typename T>
    struct type_key
    {
        static const int id;
    };
    template <typename T>
    const int type_key<T>::id = 0;

    template <typename... Args>
    struct identify
    {
        using type = std::tuple<std::type_identity<Args>...>;
    };

    struct packed_hash
    {
        template <typename T, template <typename T> typename Identity, typename U, U Value, template <typename U, U Value> typename UW> 
            requires std::is_convertible_v<T, UW<U, Value>>
        size_t predicate_hash( Identity<T> )
        {
            static std::hash<U> hasher;
            size_t              value = hasher( Value );
            return value;
        }

        template <typename T, template <typename T> typename Identity>
        size_t predicate_hash( Identity<T> )
        {
            return std::hash<const void*>{}( static_cast<const void*>( &type_key<T>::id ) );
        }

        template <size_t Index, typename... Args >
        void hash( size_t& value )
        {
            using tuplify = identify<Args...>::type;

            if constexpr ( Index < std::tuple_size_v<tuplify> )
            {
                using current_elem = std::tuple_element_t<Index, tuplify>;
                size_t hash_val    = predicate_hash( current_elem() );
                boost::hash_combine( value, hash_val );

                hash<Index + 1, Args...>( value );
            }
        }

        template <typename... Args>
        size_t operator()()
        {
            using tuplify = identify<Args...>::type;
            using first_elem = std::tuple_element_t<0, tuplify>;
            size_t value     = predicate_hash( first_elem() );

            hash<1, Args...>( value );
            return value;
        }
    };

    using memory_clean_container = std::unordered_map<const void*, bool ( * )()>;

    struct alloc_base
    {
        struct no_init
        { };

        alloc_base() = delete;

        alloc_base( no_init )
        { }

        alloc_base( alloc_base* alloc, const bool rebind, size_t hash )
        {
            if ( !rebind )
            {
                g_static_alloc.emplace( hash, alloc );
            }
        }

        bool operator==( const alloc_base& other ) const
        {
            return this == &other;
        }

        virtual memory_clean_container& get_rebind_release() const = 0;
        virtual memory_clean_container& get_rebind_purge() const   = 0;

        virtual bool purge_memory()   = 0;
        virtual bool release_memory() = 0;
    };

	template <typename T,
              typename AllocateFast  = std::integral_constant<bool, true>,
              typename UserAllocator = user_allocator,
              typename RebindFrom    = T,
              typename Mutex         = boost::details::pool::null_mutex,
              typename NextSize      = std::integral_constant<unsigned, 32U>,
              typename MaxSize       = std::integral_constant<unsigned, 0U>>
	class tag_pool_alloc : public alloc_base
	{
        template <typename Type, bool AF>
        struct crowded_tag
        { };

    public:
        inline static memory_clean_container s_rebind_release = {};
        inline static memory_clean_container s_rebind_purge   = {};

        memory_clean_container& get_rebind_release() const override
        {
            return s_rebind_release;
        }

        memory_clean_container& get_rebind_purge() const override
        {
            return s_rebind_purge;
        }

		using pool_type       = boost::singleton_pool<crowded_tag<RebindFrom, AllocateFast::value>,
                                                      sizeof( T ),
                                                      UserAllocator,
                                                      Mutex,
                                                      NextSize::value,
                                                      MaxSize::value>;
        using value_type      = T;
        using user_allocator  = typename pool_type::user_allocator;
        using mutex           = typename pool_type::mutex;
        using pointer         = T*;
        using const_pointer   = const T*;
        using reference       = T&;
        using const_reference = const T&;
        using size_type       = typename pool_type::size_type;
        using difference_type = typename pool_type::difference_type;

        struct no_init { };

        tag_pool_alloc( no_init )
            : alloc_base( alloc_base::no_init() )
        {
            pool_type::is_from( 0 );
        }

        static tag_pool_alloc* get_instanced()
        {
            static tag_pool_alloc static_alloc{ no_init() };
            return &static_alloc;
        }

        tag_pool_alloc()
            : alloc_base( get_instanced(),
                          false,
                          packed_hash().operator()< T,
                          AllocateFast,
                          UserAllocator,
                          RebindFrom,
                          Mutex,
                          NextSize,
                          MaxSize > () )
        {
            pool_type::is_from( 0 );
        }

        template <typename U>
        tag_pool_alloc(
                const tag_pool_alloc<U, AllocateFast, UserAllocator, RebindFrom, Mutex, NextSize, MaxSize>& other )
            : alloc_base( get_instanced(),
                          true,
                          packed_hash().operator()< U,
                          AllocateFast,
                          UserAllocator,
                          RebindFrom,
                          Mutex,
                          NextSize,
                          MaxSize > () )
        {
            using origin_allocator =
                    tag_pool_alloc<RebindFrom, AllocateFast, UserAllocator, RebindFrom, Mutex, NextSize, MaxSize>;
            origin_allocator::s_rebind_release.emplace( static_cast<const void*>( &type_key<pool_type>::id ), & tag_pool_alloc::static_release_memory );
            origin_allocator::s_rebind_purge.emplace( static_cast<const void*>( &type_key<pool_type>::id ), & tag_pool_alloc::static_purge_memory );
            pool_type::is_from( 0 );
        }

        template <typename U>
        struct rebind
        {
            typedef tag_pool_alloc<U, AllocateFast, UserAllocator, RebindFrom, Mutex, NextSize, MaxSize> other;
        };

        template <typename U, typename... Args>
        static void construct( U* ptr, Args&&... args )
        {
            if constexpr ( std::constructible_from<U, Args...> )
            {
                new ( ptr ) U( std::forward<Args>( args )... );
            }
        }

        static void destroy( const pointer ptr )
        {
            if constexpr ( !std::is_same_v<T, RebindFrom> )
            {
                ptr->~T();
            }
        }

        static pointer allocate( const size_type n )
        {
            if constexpr ( AllocateFast::value )
            {
                return ( pointer )pool_type::ordered_malloc( n );
            }
            else
            {
                if ( n == 1 )
                {
                    return ( pointer )pool_type::malloc();
                }
                else
                {
                    return ( pointer )pool_type::ordered_malloc( n );
                }
            }
        }
        static pointer allocate( const size_type n, const void* const )
        {
            return allocate( n );
        }

        static void deallocate( const pointer ptr )
        {
            if constexpr ( AllocateFast::value )
            {
                pool_type::ordered_free( ptr );
            }
            else
            {
                pool_type::free( ptr );
            }
        }
        static void deallocate( const pointer ptr, const size_type n )
        {
            if constexpr ( AllocateFast::value )
            {
                pool_type::ordered_free( ptr, n );
            }
            else
            {
                if ( n == 1 )
                {
                    pool_type::free( ptr );
                }
                else
                {
                    pool_type::free( ptr, n );
                }
            }
        }

        bool release_memory() override
        {
            return pool_type::release_memory();
        }

        bool purge_memory() override
        {
            return pool_type::purge_memory();
        }

        static bool static_release_memory()
        {
            return get_instanced()->release_memory();
        }

        static bool static_purge_memory()
        {
            return get_instanced()->purge_memory();
        }
	};

    // Alignment: use e.g. std::integral_constant<size_t, 64> for L1 cache-line alignment.
    template <class T, typename Alignment = std::integral_constant<size_t, 8>, typename RebindFrom = T>
    class aligned_alloc : public alloc_base
    {
        static_assert( Alignment::value );

    private:
        std::unordered_map<T*, size_t> m_allocated_ptr_;

        static constexpr size_t alloc_alignment()
        {
            constexpr size_t a = Alignment::value;
            constexpr size_t t = boost::alignment_of<T>::value;
            return a >= t ? a : t;
        }
        static size_t align_up( size_t size, size_t alignment )
        {
            return ( size + alignment - 1 ) & ~( alignment - 1 );
        }

    public:
        inline static memory_clean_container s_rebind_release = {};
        inline static memory_clean_container s_rebind_purge   = {};

        memory_clean_container& get_rebind_release() const override
        {
            return s_rebind_release;
        }

        memory_clean_container& get_rebind_purge() const override
        {
            return s_rebind_purge;
        }

        using pool_type = aligned_alloc<T, Alignment, RebindFrom>;
        typedef T                                                    value_type;
        typedef T*                                                   pointer;
        typedef const T*                                             const_pointer;
        typedef void*                                                void_pointer;
        typedef const void*                                          const_void_pointer;
        typedef typename boost::alignment::detail::add_lvalue_reference<T>::type       reference;
        typedef typename boost::alignment::detail::add_lvalue_reference<const T>::type const_reference;
        typedef std::size_t                                          size_type;
        typedef std::ptrdiff_t                                       difference_type;
        typedef boost::alignment::detail::true_type propagate_on_container_move_assignment;
        typedef boost::alignment::detail::true_type is_always_equal;


        template <class U>
        struct rebind
        {
            typedef aligned_alloc<U, Alignment, RebindFrom> other;
        };

        struct no_init { };

        static aligned_alloc* get_instanced()
        {
            static aligned_alloc static_alloc{ no_init() };
            return &static_alloc;
        }

        aligned_alloc( no_init ) : alloc_base( alloc_base::no_init{} )
        { }

        aligned_alloc()
            : alloc_base(
                      get_instanced(),
                      false,
                      packed_hash().operator()<T, Alignment, RebindFrom>() )
        {
        }

        template <class U>
        aligned_alloc( const aligned_alloc<U, Alignment, RebindFrom>& ) noexcept
            : alloc_base( get_instanced(), true, packed_hash().operator()<U, Alignment, RebindFrom>() )
        {
            using origin_allocator = aligned_alloc<RebindFrom, Alignment, RebindFrom>;
            origin_allocator::s_rebind_release.emplace( static_cast<const void*>( &type_key<pool_type>::id ), & aligned_alloc::static_release_memory );
            origin_allocator::s_rebind_purge.emplace( static_cast<const void*>( &type_key<pool_type>::id ), &aligned_alloc::static_purge_memory );
        }

        pointer allocate( size_t size, const void* = 0 )
        {
            if ( size == 0 )
            {
                return 0;
            }
            const size_t region_bytes = sizeof( T ) * size;
            const size_t aligned_region = align_up( region_bytes, Alignment::value );
            void* p = boost::alignment::aligned_alloc( alloc_alignment(), aligned_region );
            if ( !p )
            {
                boost::alignment::detail::throw_exception( std::bad_alloc() );
            }

            m_allocated_ptr_.insert_or_assign( static_cast<T*>( p ), size );
            return static_cast<T*>( p );
        }

        void deallocate( pointer ptr, size_t )
        {
            boost::alignment::aligned_free( ptr );

            if ( m_allocated_ptr_.contains( ptr ) )
            {
                m_allocated_ptr_.erase( ptr );
            }
        }

        template <class U>
        void construct( U* ptr )
        {
            ::new ( ( void* )ptr ) U();
        }

        template <class U>
        void destroy( U* ptr )
        {
            ( void )ptr;
            ptr->~U();
        }

        bool release_memory() override
        {
            bool removed = false;
            for ( const auto& [ ptr, size ] : m_allocated_ptr_ )
            {
                for ( size_t i = 0; i < size; ++i )
                {
                    destroy<T>( reinterpret_cast<T*>( ( uintptr_t )ptr + ( sizeof( T ) * i ) ) );
                    removed = true;
                }
            }
            return removed;
        }

        bool purge_memory() override
        {
            std::vector<std::pair<pointer, size_t>> copy( m_allocated_ptr_.begin(), m_allocated_ptr_.end() );
            for ( const auto& [ ptr, size ] : copy )
            {
                deallocate( ptr, size );
            }
            return !copy.empty();
        }

        static bool static_release_memory()
        {
            return get_instanced()->release_memory();
        }

        static bool static_purge_memory()
        {
            return get_instanced()->purge_memory();
        }
    };

	template <typename KeyType, typename ValueType>
	using u_fast_pool_allocator = tag_pool_alloc<std::pair<const KeyType, ValueType>, std::integral_constant<bool, true>>;

	template <typename ValueType>
	using u_fast_pool_allocator_single = tag_pool_alloc<ValueType, std::integral_constant<bool, true>>;

	template <typename ValueType>
	using u_align_allocator = aligned_alloc<ValueType>;

	template <typename ValueType>
	using u_pool_allocator_single = tag_pool_alloc<ValueType, std::integral_constant<bool, false>>;

	template <typename KeyType>
	using fast_pool_set = std::set<KeyType, std::less<KeyType>, u_fast_pool_allocator_single<KeyType>>;

	template <typename KeyType, typename ValueType>
	using fast_pool_unordered_map = std::unordered_map<KeyType, ValueType, std::hash<KeyType>, std::equal_to<KeyType>, u_fast_pool_allocator<KeyType, ValueType>>;

	template <typename KeyType, typename ValueType>
	using fast_pool_map = std::map<KeyType, ValueType, std::less<KeyType>, u_fast_pool_allocator<KeyType, ValueType>>;

	template <typename ValueType>
	using pool_queue = std::queue<ValueType, std::deque<ValueType, u_pool_allocator_single<ValueType>>>;

	template <typename ValueType>
	using aligned_vector = std::vector<ValueType, u_align_allocator<ValueType>>;
}
#pragma once
#include <set>
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

extern ENGINE_MEMORY_API std::unordered_map<size_t, const Engine::alloc_base*> g_static_alloc;

namespace Engine 
{
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
            size_t value = typeid( T ).hash_code();
            return value;
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

    struct alloc_base
    {
        struct no_init
        { };

        alloc_base() = delete;

        alloc_base( no_init )
        { }

        alloc_base( const alloc_base* alloc, const bool rebind, size_t hash )
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

        virtual std::unordered_set<void ( * )()>& get_rebind_release() const = 0;
        virtual std::unordered_set<void ( * )()>& get_rebind_purge() const   = 0;

        virtual void purge_memory() const   = 0;
        virtual void release_memory() const = 0;
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
        inline static std::unordered_set<void(*)()> s_rebind_release = {};
        inline static std::unordered_set<void(*)()> s_rebind_purge   = {};

        std::unordered_set<void (*)()>& get_rebind_release() const override
        {
            return s_rebind_release;
        }

        std::unordered_set<void ( * )()>& get_rebind_purge() const override
        {
            return s_rebind_release;
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

        static const tag_pool_alloc* get_instanced()
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
            origin_allocator::s_rebind_release.emplace( &tag_pool_alloc::static_release_memory );
            origin_allocator::s_rebind_purge.emplace( &tag_pool_alloc::static_purge_memory );
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

        void release_memory() const override
        {
            pool_type::release_memory();
        }

        void purge_memory() const override
        {
            pool_type::purge_memory();
        }

        static void static_release_memory()
        {
            get_instanced()->release_memory();
        }

        static void static_purge_memory()
        {
            get_instanced()->purge_memory();
        }
	};

    /*
    template <class T, std::size_t Alignment, typename RebindFrom = T>
    class aligned_alloc : public alloc_base, public boost::alignment::aligned_allocator<T, Alignment>
    {
        static_assert( Alignment );

    public:
        inline static std::unordered_set<void ( * )()> s_rebind_release = {};
        inline static std::unordered_set<void ( * )()> s_rebind_purge   = {};

        std::unordered_set<void (*)()>& get_rebind_release() const override
        {
            return s_rebind_release;
        }

        std::unordered_set<void (*)()>& get_rebind_purge() const override
        {
            return s_rebind_purge;
        }

        using pool_type = boost::alignment::aligned_allocator<T, Alignment>;

        template <class U>
        struct rebind
        {
            typedef aligned_alloc<U, Alignment, RebindFrom> other;
        };

        struct no_init { };

        static const aligned_alloc* get_instanced()
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
            origin_allocator::s_rebind_release.emplace( &aligned_alloc::static_release_memory );
            origin_allocator::s_rebind_purge.emplace( &aligned_alloc::static_purge_memory );
        }

        void release_memory() const override
        {
            for ( void ( *func )() : s_rebind_release )
            {
                func();
            }
        }

        void purge_memory() const override
        {
            for ( void ( *func )() : s_rebind_purge )
            {
                func();
            }
        }

        static void static_release_memory()
        {
            get_instanced()->release_memory();
        }

        static void static_purge_memory()
        {
            get_instanced()->release_memory();
        }
    };
    */

	inline static constexpr size_t g_cache_alignment = 8;

	template <typename KeyType, typename ValueType>
	using u_fast_pool_allocator = tag_pool_alloc<std::pair<const KeyType, ValueType>, std::integral_constant<bool, true>>;

	template <typename ValueType>
	using u_fast_pool_allocator_single = tag_pool_alloc<ValueType, std::integral_constant<bool, true>>;

	template <typename ValueType>
	using u_align_allocator = boost::alignment::aligned_allocator<ValueType, g_cache_alignment>;

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
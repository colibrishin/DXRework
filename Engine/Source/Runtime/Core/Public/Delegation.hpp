#pragma once
#include <cstring>
#include <memory>
#include <new>
#include <tuple>
#include <type_traits>
#include <utility>

#include "TypeLibrary.h"
#include "DelegateCore.h"

// Delegates call registered listeners on Broadcast. The listener list and every change to it is Core code
// (DelegateState); these templates only adapt the listener types.
//
// Delegate            one thread at a time (the thread that registered its listeners); copying it gives an empty
//                     delegate, moving it moves the listeners (and the handles follow).
// ConcurrentDelegate  any thread; listeners may run on several threads at once. Register with ListenScoped only.
//
// Listeners may register, remove themselves or broadcast again from inside a callback. A listener removed during
// a broadcast is not called after the removal, including by the broadcast in progress.
// Do not register or destroy a handle while holding a lock that a listener may take: removal waits for the calls
// in progress on other threads. For the same reason, a listener must not destroy the handle of a listener that may
// be running on another thread and waiting for this one (two listeners removing each other at once deadlock).

namespace Engine::DelegateDetail
{
    template <typename F, typename... Args>
    struct Ops
    {
        static void Invoke( void* callable, const void* arguments )
        {
            std::apply( *static_cast<F*>( callable ), *static_cast<const std::tuple<const Args&...>*>( arguments ) );
        }

        static void Destroy( void* callable ) noexcept
        {
            static_cast<F*>( callable )->~F();
        }

        static constexpr DelegateOps value{ &Invoke, &Destroy };
    };

    template <typename F>
    void* Construct( F&& function )
    {
        using Callable = std::decay_t<F>;
        void* memory   = DelegateState::AllocateCallable( sizeof( Callable ), alignof( Callable ) );
        try
        {
            new ( memory ) Callable( std::forward<F>( function ) );
        }
        catch ( ... )
        {
            DelegateState::FreeCallable( memory, alignof( Callable ) );
            throw;
        }
        return memory;
    }

    // Identity of a function or member function pointer, compared by value. MSVC member function pointers of
    // classes with more than one base are 16 bytes: the code address, a 4-byte this adjustment and 4 bytes of
    // padding whose value is not defined, so only the first 12 bytes are part of the key.
    template <typename P>
    void KeyOf( const P pointer, uintptr_t ( &key )[ 2 ] )
    {
        static_assert( sizeof( P ) == sizeof( void* ) || sizeof( P ) == 2 * sizeof( void* ),
                       "Delegate: unsupported member function pointer representation" );
        constexpr size_t significant = sizeof( P ) == sizeof( void* ) ? sizeof( void* ) : sizeof( void* ) + sizeof( int );
        key[ 0 ] = key[ 1 ] = 0;
        std::memcpy( key, &pointer, significant );
    }

    template <typename F, typename... Args>
    uint64_t AddCallable( DelegateState&                   state,
                          const Weak<Abstracts::Entity>&   owner,
                          const uintptr_t                  ( &key )[ 2 ],
                          const bool                       deduplicate,
                          F&&                              function )
    {
        using Callable = std::decay_t<F>;
        return state.Add( owner, key, deduplicate, &Ops<Callable, Args...>::value, Construct( std::forward<F>( function ) ),
                          alignof( Callable ) );
    }

    template <typename F, typename... Args>
    concept Listener = std::is_invocable_v<std::decay_t<F>&, const Args&...>;
}

template <typename... Args>
struct Delegate
{
public:
    INLINE_COMPILE_TIME_TYPENAME_NON_ENTITY( Delegate<Args...> )

    using base_class_type = Engine::Abstracts::Entity;

    template <typename T>
    using weak_this_type = Engine::Weak<T>;

    using raw_function_type = void ( * )( Args... );

    Delegate() = default;

    // A copy (a cloned component or object) starts without listeners: the original's listeners belong to it.
    Delegate( const Delegate& ) noexcept {}

    // Assigning keeps this delegate's own listeners.
    Delegate& operator=( const Delegate& ) noexcept
    {
        return *this;
    }

    Delegate( Delegate&& ) noexcept            = default;
    Delegate& operator=( Delegate&& ) noexcept = default;

    template <typename T> requires ( std::is_base_of_v<base_class_type, T> )
    void Listen( const weak_this_type<T>& this_pointer, void ( T::*function )( Args... ) )
    {
        ListenMember( this_pointer, function );
    }

    template <typename T> requires ( std::is_base_of_v<base_class_type, T> )
    void Listen( const weak_this_type<T>& this_pointer, void ( T::*function )( Args... ) const )
    {
        ListenMember( this_pointer, function );
    }

    // A plain function or a callable with no owner; it stays until Remove (plain functions) or for the delegate's
    // lifetime. Use ListenScoped when the listener must be removable or comes from another module.
    template <typename FunctionT> requires Engine::DelegateDetail::Listener<FunctionT, Args...>
    void Listen( FunctionT&& func )
    {
        uintptr_t key[ 2 ]{};
        if constexpr ( std::is_convertible_v<std::decay_t<FunctionT>, raw_function_type> &&
                       std::is_pointer_v<std::decay_t<FunctionT>> )
        {
            Engine::DelegateDetail::KeyOf( static_cast<raw_function_type>( func ), key );
        }
        Engine::DelegateDetail::AddCallable<FunctionT, Args...>( State(), {}, key, false, std::forward<FunctionT>( func ) );
    }

    // Registers a callable and returns the handle that removes it.
    template <typename FunctionT> requires Engine::DelegateDetail::Listener<FunctionT, Args...>
    [[nodiscard]] Engine::DelegateHandle ListenScoped( FunctionT&& func )
    {
        constexpr uintptr_t key[ 2 ]{};
        const uint64_t id = Engine::DelegateDetail::AddCallable<FunctionT, Args...>( State(), {}, key, false,
                                                                                    std::forward<FunctionT>( func ) );
        return { m_state_, id };
    }

    void Broadcast( Args... args )
    {
        if ( m_state_ && m_state_->ListenerCount() != 0 )
        {
            const std::tuple<const Args&...> arguments( args... );
            m_state_->Invoke( &arguments );
        }
    }

    template <typename T> requires ( std::is_base_of_v<base_class_type, T> )
    void Remove( const weak_this_type<T>& this_pointer, void ( T::*function )( Args... ) )
    {
        RemoveMember( this_pointer, function );
    }

    template <typename T> requires ( std::is_base_of_v<base_class_type, T> )
    void Remove( const weak_this_type<T>& this_pointer, void ( T::*function )( Args... ) const )
    {
        RemoveMember( this_pointer, function );
    }

    // Removes a plain function registered with Listen. Other callables are removed through their handle.
    template <typename FunctionT>
        requires std::is_pointer_v<FunctionT> && std::is_convertible_v<FunctionT, raw_function_type>
    void Remove( const FunctionT func )
    {
        if ( m_state_ && func )
        {
            uintptr_t key[ 2 ];
            Engine::DelegateDetail::KeyOf( func, key );
            m_state_->RemoveOwnerless( key );
        }
    }

    [[nodiscard]] size_t ListenerCount() const noexcept
    {
        return m_state_ ? m_state_->ListenerCount() : 0;
    }

private:
    Engine::DelegateState& State()
    {
        if ( !m_state_ )
        {
            m_state_ = Engine::DelegateState::Create( Engine::DelegateState::Threading::Single );
        }
        return *m_state_;
    }

    template <typename T, typename F>
    void ListenMember( const weak_this_type<T>& this_pointer, const F function )
    {
        if ( this_pointer.empty() )
        {
            return;
        }

        uintptr_t key[ 2 ];
        Engine::DelegateDetail::KeyOf( function, key );
        Engine::DelegateDetail::AddCallable<decltype( MemberCall( this_pointer, function ) ), Args...>(
                State(), Engine::Weak<base_class_type>( this_pointer ), key, true, MemberCall( this_pointer, function ) );
    }

    template <typename T, typename F>
    void RemoveMember( const weak_this_type<T>& this_pointer, const F function )
    {
        if ( m_state_ )
        {
            uintptr_t key[ 2 ];
            Engine::DelegateDetail::KeyOf( function, key );
            m_state_->RemoveByKey( Engine::Weak<base_class_type>( this_pointer ), key );
        }
    }

    template <typename T, typename F>
    static auto MemberCall( const weak_this_type<T>& this_pointer, const F function )
    {
        return [ this_pointer, function ]( const Args&... arguments )
        {
            if ( const Engine::Strong<T> locked = this_pointer.lock() )
            {
                ( locked.get()->*function )( arguments... );
            }
        };
    }

    std::shared_ptr<Engine::DelegateState> m_state_;
};

template <typename... Args>
struct ConcurrentDelegate
{
public:
    INLINE_COMPILE_TIME_TYPENAME_NON_ENTITY( ConcurrentDelegate<Args...> )

    ConcurrentDelegate() : m_state_( Engine::DelegateState::Create( Engine::DelegateState::Threading::Concurrent ) ) {}

    ConcurrentDelegate( const ConcurrentDelegate& )            = delete;
    ConcurrentDelegate& operator=( const ConcurrentDelegate& ) = delete;

    // Registers a callable and returns the handle that removes it. The callable may run on several threads at once.
    template <typename FunctionT> requires Engine::DelegateDetail::Listener<FunctionT, Args...>
    [[nodiscard]] Engine::DelegateHandle ListenScoped( FunctionT&& func )
    {
        constexpr uintptr_t key[ 2 ]{};
        const uint64_t id = Engine::DelegateDetail::AddCallable<FunctionT, Args...>( *m_state_, {}, key, false,
                                                                                    std::forward<FunctionT>( func ) );
        return { m_state_, id };
    }

    void Broadcast( Args... args )
    {
        if ( m_state_->ListenerCount() != 0 )
        {
            const std::tuple<const Args&...> arguments( args... );
            m_state_->Invoke( &arguments );
        }
    }

    [[nodiscard]] size_t ListenerCount() const noexcept
    {
        return m_state_->ListenerCount();
    }

private:
    std::shared_ptr<Engine::DelegateState> m_state_;
};

#define DEFINE_DELEGATE(Name, ...) \
struct Delegate##Name : public Delegate<__VA_ARGS__> \
{ \
	INLINE_COMPILE_TIME_TYPENAME_NON_ENTITY(Delegate##Name) \
}; \

#define DEFINE_CONCURRENT_DELEGATE(Name, ...) \
struct Delegate##Name : public ConcurrentDelegate<__VA_ARGS__> \
{ \
	INLINE_COMPILE_TIME_TYPENAME_NON_ENTITY(Delegate##Name) \
}; \

#pragma once
#include <functional>
#include <map>
#include "TypeLibrary.h"

namespace Engine
{
	template <size_t... Indices> struct indices {};

	template <size_t N, size_t... Following>
	struct build_indices : build_indices<N -1, N-1, Following...> {};

	template <size_t... Indices>
	struct build_indices<0, Indices...> : indices<Indices...> {};

    template <typename T, typename... Args>
    struct proxy_call
    {
        static void invoke( managed_weak_ptr<T> this_pointer, void ( T::*function )( Args... ), Args... args )
        {
            
        }

		static void invoke( managed_weak_ptr<T> this_pointer, void ( T::*function )( Args... ) const, Args... args )
        {
            if ( const managed_shared_ptr<T>& locked = this_pointer.lock() )
            {
                std::bind_front( function, locked.get() )( args... );
            }
        }
    };

	template <size_t... Indices, typename T, typename... Args>
	auto mem_bind_impl(indices<Indices...>, T* this_pointer, void(T::*function)(Args...))
	{
		return std::bind(function, this_pointer, std::_Ph<Indices + 1>{}...);
	}

	template <size_t... Indices, typename T, typename... Args>
	auto mem_bind_impl(indices<Indices...>, const T* this_pointer, void(T::*function)(Args...) const)
	{
		return std::bind(function, this_pointer, std::_Ph<Indices + 1>{}...);
	}

	template <size_t... Indices, typename T, typename... Args>
    auto managed_mem_bind_impl( indices<Indices...>, managed_weak_ptr<T> this_pointer, void ( T::*function )( Args... ) )
    {
        return std::bind(
                []( managed_weak_ptr<T> p, void ( T::*f )( Args... ), Args... ags )
                {
                    if ( const managed_shared_ptr<T>& locked = p.lock() )
                    {
                        std::bind_front( f, locked.get() )( ags... );
                    }
                },
                this_pointer,
                function,
                std::_Ph<Indices + 1>{}... );
    }

	template <size_t... Indices, typename T, typename... Args>
    auto managed_mem_bind_impl( indices<Indices...>,
                                managed_weak_ptr<T> this_pointer,
                                void ( T::*function )( Args... ) const )
    {
        return std::bind(
                []( managed_weak_ptr<T> p, void ( T::*f )( Args... ) const, Args... ags )
                {
                    if ( const managed_shared_ptr<T>& locked = p.lock() )
                    {
                        std::bind_front( f, locked.get() )( ags... );
                    }
                },
                this_pointer,
                function,
                std::_Ph<Indices + 1>{}... );
    }

	template <typename T, typename... Args>
    auto mem_bind( managed_weak_ptr<T> this_pointer, void ( T::*function )( Args... ) )
    {
        return managed_mem_bind_impl( build_indices<sizeof...( Args )>{}, this_pointer, function );
    }

	template <typename T, typename... Args>
    auto mem_bind( managed_weak_ptr<T> this_pointer, void ( T::*function )( Args... ) const )
    {
        return managed_mem_bind_impl( build_indices<sizeof...( Args )>{}, this_pointer, function );
    }

	template <typename T, typename... Args>
	auto mem_bind(T* this_pointer, void(T::*function)(Args...))
	{
		return mem_bind_impl(build_indices<sizeof...(Args)>{}, this_pointer, function);
	}

	template <typename T, typename... Args>
	auto mem_bind(T* this_pointer, void(T::*function)(Args...) const)
	{
		return mem_bind_impl(build_indices<sizeof...(Args)>{}, this_pointer, function);
	}
}

template <typename... Args>
struct Delegate
{
public:
	INLINE_COMPILE_TIME_TYPENAME_NON_ENTITY(Delegate<Args...>)

	using address_type = uintptr_t;

	using base_class_type = Engine::Abstracts::Entity;

	template <typename T>
	using strong_this_type = Engine::Strong<T>;

	template <typename T>
	using weak_this_type = Engine::Weak<T>;
	using func_ptr_type = address_type;

	using bucket_type = std::pair<weak_this_type<base_class_type>, func_ptr_type>;
	using raw_function_type = void(*)(Args...);
	using function_type = std::function<void(Args...)>;

	template <typename T> requires (std::is_base_of_v<base_class_type, T>)
	void Listen(const strong_this_type<T>& this_pointer, void(T::*function)(Args...))
	{
		if (this_pointer)
		{
            function_type func = Engine::mem_bind( this_pointer.to_weak(), function );
			m_listener_.emplace(bucket_type{ this_pointer, reinterpret_cast<address_type&>( function ) }, func);
		}
	}

	// const function
	template <typename T> requires (std::is_base_of_v<base_class_type, T>)
	void Listen(const strong_this_type<T>& this_pointer, void(T::*function)(Args...) const)
	{
		if (this_pointer)
		{
            function_type func = Engine::mem_bind( this_pointer.to_weak(), function );
            m_listener_.emplace( bucket_type{ this_pointer, reinterpret_cast<address_type&>( function ) }, func );
		}
	}

	template <typename FunctionT>
        requires std::is_convertible_v<FunctionT, function_type>
    void Listen( const FunctionT& func )
    {
        function_type wrapper       = func;
        address_type  function_addr = reinterpret_cast<address_type>( wrapper.template target<FunctionT>() );

        m_listener_.emplace( bucket_type{ {}, function_addr }, std::move( wrapper ) );
    }

	void Broadcast(Args... args)
	{
		for (typename decltype(m_listener_)::iterator it = m_listener_.begin(); it != m_listener_.end();)
		{
			const bucket_type& key = it->first;
			const function_type& value = it->second;

			const bool weak_valid  = !key.first.empty() && !key.first.expired();
			const bool static_func = key.first.empty() && value;

			if (weak_valid || static_func)
			{
				// does not forward, need to reuse the variable for each invocations.
				value(args...);
				++it;
			}
			else
			{
				it = m_listener_.erase(it);
			}
		}
	}

	template <typename T> requires (std::is_base_of_v<base_class_type, T>)
	void Remove(const strong_this_type<T>& this_pointer, void(T::*function)(Args...))
	{
		const bucket_type key{ this_pointer, reinterpret_cast<address_type>(&function) };

		if (m_listener_.contains(key))
		{
			m_listener_.erase(key);
		}
	}

	template <typename FunctionT> requires std::is_convertible_v<FunctionT, function_type>
	void Remove( const FunctionT& func )
    {
        function_type wrapper       = func;
        address_type  function_addr = reinterpret_cast<address_type>( wrapper.template target<FunctionT>() );

        const bucket_type key { {}, function_addr };
        
		if ( m_listener_.contains( key ) )
		{
            m_listener_.erase( key );
		}
    }

private:
	std::map<bucket_type, function_type> m_listener_{};

};

#define DEFINE_DELEGATE(Name, ...) \
struct Delegate##Name : public Delegate<__VA_ARGS__> \
{ \
	INLINE_COMPILE_TIME_TYPENAME_NON_ENTITY(Delegate##Name) \
}; \

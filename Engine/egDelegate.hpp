#pragma once
#include <functional>
#include <ranges>

template <typename... Args>
struct Delegate
{
public:
	using address_type = LONG_PTR;
	using hash_type = uint64_t;
	using bucket_type = std::pair<address_type, hash_type>;
	using function_type = std::function<void(Args...)>;

	template <typename T>
	void Listen(T* this_pointer, void(T::*function)(Args...))
	{
		size_t hash = typeid(function).hash_code();
		function_type func = mem_bind(this_pointer, function);

		// todo: garbage collection
		m_listener_.emplace(bucket_type{reinterpret_cast<address_type>(this_pointer), hash}, func);
	}

	void Listen(void(*function)(Args...))
	{
		size_t hash = typeid(function).hash_code();

		m_listener_.emplace(bucket_type{ reinterpret_cast<address_type>(nullptr), hash }, function);
	}

	void Broadcast(Args&&... args)
	{
		for (const function_type& func : m_listener_ | std::views::values)
		{
			func(std::forward<Args>(args)...);
		}
	}

	template <typename T>
	void Remove(T* this_pointer, void(T::*function)(Args...))
	{
		const bucket_type key{ this_pointer, typeid(function).hash_code() };

		if (m_listener_.contains(key))
		{
			m_listener_.erase(key);
		}
	}

	void Remove(void(*function)(Args...))
	{
		const bucket_type key{ reinterpret_cast<address_type>(nullptr), typeid(function).hash_code() };

		if (m_listener_.contains(key))
		{
			m_listener_.erase(key);
		}
	}

private:
	template <size_t... Indices, typename T>
	auto mem_bind_impl(T* this_pointer, void(T::*function)(Args...))
	{
		return std::bind(function, this_pointer, std::_Ph<Indices>{}...);
	}

	template <typename T>
	auto mem_bind(T* this_pointer, void(T::*function)(Args...))
	{
		return mem_bind_impl<sizeof...(Args)>(this_pointer, function);
	}

	std::map<bucket_type, function_type> m_listener_{};

};

#define DEFINE_DELEGATE(Name, ...) \
struct Delegate##Name : public Delegate<__VA_ARGS__> {};

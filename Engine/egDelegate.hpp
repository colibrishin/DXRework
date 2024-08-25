#pragma once
#include <functional>
#include <ranges>

template <typename... Args>
struct Delegate
{
public:
	using function_type = std::function<void(Args...)>;

	template <typename T>
	void Listen(T* this_pointer, void(T::*function)(Args...))
	{
		size_t hash = typeid(function).hash_code();
		std::function<void(Args...)> func = mem_bind(this_pointer, function);

		m_listener_.emplace(hash, func);
	}

	void Broadcast(Args&&... args)
	{
		for (const function_type& func : m_listener_ | std::views::values)
		{
			func(std::forward<Args>(args)...);
		}
	}

	void Remove(const function_type& function)
	{
		if (m_listener_.contains(typeid(function).hash_code()))
		{
			m_listener_.erase(typeid(function).hash_code());
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

	std::unordered_map<size_t, function_type> m_listener_{};

};

#define DEFINE_DELEGATE(Name, ...) \
struct Delegate##Name : public Delegate<__VA_ARGS__> {};

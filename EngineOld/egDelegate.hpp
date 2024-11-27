#pragma once
#include <functional>
#include <ranges>

template <typename... Args>
struct Delegate
{
public:
	using address_type = LONG_PTR;

	using base_class_type = Engine::Abstract::Entity;

	template <typename T>
	using strong_this_type = boost::shared_ptr<T>;

	template <typename T>
	using weak_this_type = boost::weak_ptr<T>;
	using func_ptr_type = address_type;

	using bucket_type = std::pair<weak_this_type<base_class_type>, func_ptr_type>;
	using function_type = std::function<void(Args...)>;

	template <typename T> requires (std::is_base_of_v<base_class_type, T>)
	void Listen(const strong_this_type<T>& this_pointer, void(T::*function)(Args...))
	{
		if (this_pointer)
		{
			function_type func = Engine::mem_bind(this_pointer.get(), function);
			m_listener_.emplace(bucket_type{ this_pointer, reinterpret_cast<address_type>(&function) }, func);
		}
	}

	// const function
	template <typename T> requires (std::is_base_of_v<base_class_type, T>)
	void Listen(const strong_this_type<T>& this_pointer, void(T::*function)(Args...) const)
	{
		if (this_pointer)
		{
			function_type func = Engine::mem_bind(this_pointer.get(), function);
			m_listener_.emplace(bucket_type{ this_pointer, reinterpret_cast<address_type>(&function) }, func);
		}
	}

	void Listen(void(*function)(Args...))
	{
		m_listener_.emplace(bucket_type{ {}, reinterpret_cast<address_type>(&function) }, function);
	}

	void Broadcast(Args... args)
	{
		for (typename decltype(m_listener_)::iterator it = m_listener_.begin(); it != m_listener_.end();)
		{
			const bucket_type& key = it->first;
			const function_type& value = it->second;

			const strong_this_type<base_class_type>& locked = key.first.lock();

			const bool       weak_valid  = !key.first.empty() && locked;
			const bool static_func = key.first.empty() && value;

			if (weak_valid || static_func)
			{
				value(std::forward<Args>(args)...);
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

	void Remove(void(*function)(Args...))
	{
		const bucket_type key{ {}, reinterpret_cast<address_type>(&function) };

		if (m_listener_.contains(key))
		{
			m_listener_.erase(key);
		}
	}

private:
	std::map<bucket_type, function_type> m_listener_{};

};

#define DEFINE_DELEGATE(Name, ...) \
struct Delegate##Name : public Delegate<__VA_ARGS__> {};

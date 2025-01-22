#pragma once
#define EMPTY
#define DLLIMPORT __declspec(dllimport)
#define DLLEXPORT __declspec(dllexport)
#include <string_view>
// If you can't use C++17's standard library, you'll need to use the GSL 
// string_view or implement your own struct (which would not be very difficult,
// since we only need a few methods here)

template <typename T> constexpr std::string_view compile_time_type_name();

template <>
constexpr std::string_view compile_time_type_name<void>()
{ return "void"; }

namespace detail {

using type_name_prober = void;

template <typename T>
constexpr std::string_view wrapped_type_name() 
{
#ifdef __clang__
    return __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
    return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    return __FUNCSIG__;
#else
#error "Unsupported compiler"
#endif
}

constexpr std::size_t wrapped_type_name_prefix_length() { 
    return wrapped_type_name<type_name_prober>().find(compile_time_type_name<type_name_prober>()); 
}

constexpr std::size_t wrapped_type_name_suffix_length() { 
    return wrapped_type_name<type_name_prober>().length() 
        - wrapped_type_name_prefix_length() 
        - compile_time_type_name<type_name_prober>().length();
}

} // namespace detail

template <typename T>
constexpr std::string_view compile_time_type_name() {
    constexpr auto wrapped_name = detail::wrapped_type_name<T>();
    constexpr auto prefix_length = detail::wrapped_type_name_prefix_length();
    constexpr auto suffix_length = detail::wrapped_type_name_suffix_length();
    constexpr auto type_name_length = wrapped_name.length() - prefix_length - suffix_length;
    return wrapped_name.substr(prefix_length, type_name_length);
}

#define INLINE_COMPILE_TIME_TYPENAME(Type) \
	static std::string_view StaticTypeName() \
	{ \
		static constexpr std::string_view type_name = compile_time_type_name<Type>(); \
		return type_name; \
	}
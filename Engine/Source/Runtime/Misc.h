#pragma once
#define EMPTY
#define DLLIMPORT __declspec(dllimport)
#define DLLEXPORT __declspec(dllexport)

#include <algorithm>
#include <array>
#include <vector>
#include <cstddef>
#include <string_view>

struct typename_prober
{
public:
    template <typename T>
	[[nodiscard]] static constexpr std::string_view probe()
	{
#ifndef _MSC_VER
	    return __PRETTY_FUNCTION__;
#else
	    return __FUNCSIG__;
#endif
	}

private:
    static constexpr std::string_view VoidTypeName = probe<void>();

public:
    static constexpr size_t PrefixCount = VoidTypeName.find("void");
    static constexpr size_t RemovalCount = VoidTypeName.size() - 4;
	static_assert(PrefixCount != std::string::npos, "Unable to determine the type name format on this compiler.");
};

using HashType = int const*;

template <typename T>
struct type_hash
{
private:
	static constexpr int unique{};
public:
	static constexpr HashType value { &unique };
};

template <typename T>
struct polymorphic_type_hash
{
	static constexpr size_t upcast_count = 0;
	static constexpr std::array<HashType, upcast_count> upcast_array {};

	static bool is_base_of(HashType base)
	{
		return false;
	}
};

template <>
struct polymorphic_type_hash<void>
{
	static constexpr size_t upcast_count = 1;
	static constexpr std::array<HashType, upcast_count> upcast_array { type_hash<void>::value };
};

#define POLYMORPHIC_MANAGER_TYPE_MAP(API, Type) \
POLYMORPHIC_TYPE_MAP(API, Engine::Abstracts::Singleton<##Type##>, Engine::Abstracts::SingletonBase) \
POLYMORPHIC_TYPE_MAP(API, Type, Engine::Abstracts::Singleton<##Type##>)

#define POLYMORPHIC_TYPE_MAP(API, Type, Base) \
template <> \
struct API polymorphic_type_hash<##Type##> \
{ \
	static constexpr size_t upcast_count = 1 + polymorphic_type_hash<##Base##>::upcast_count; \
	static constexpr std::array<HashType, upcast_count> upcast_array = [] \
	{ \
		std::array<HashType, upcast_count> ret { type_hash<##Type##>::value }; \
		std::copy_n(polymorphic_type_hash<##Base##>::upcast_array.begin(),  polymorphic_type_hash<##Base##>::upcast_array.size(), ret.data() + 1); \
		std::sort(ret.begin(), ret.end()); \
		return ret; \
	}(); \
	static bool is_base_of(HashType base) \
	{ \
		return std::ranges::binary_search(upcast_array, base); \
	} \
};

template <typename T>
struct static_type_name
{
private:
    static constexpr std::string_view PlainTypeName = typename_prober::probe<T>();
    static constexpr size_t TypeNameLengthNullTrailling = PlainTypeName.size() - typename_prober::RemovalCount + 1;
    static constexpr size_t TypeNameStartOffset = typename_prober::PrefixCount;

    using TypeNameStorageT = std::array<char, TypeNameLengthNullTrailling>;

    static consteval TypeNameStorageT EvalTypeNameImpl()
    {
	    TypeNameStorageT ret{};
        std::copy_n(PlainTypeName.data() + TypeNameStartOffset, ret.size() - 1, ret.data());
        return ret;
    }

    static constexpr TypeNameStorageT TypeNameStorage = EvalTypeNameImpl();

public:
    static constexpr std::string_view name()
    {
	    // MSVC tested, others are not tested.
	    // Find the begining of the namespace from the end
	    constexpr auto it = std::find(TypeNameStorage.rbegin(), TypeNameStorage.rend(), ':');
	    if constexpr (it == TypeNameStorage.rend())
	    {
	        // No namespace
		    return {TypeNameStorage.data(), TypeNameStorage.size() - 1};
	    }

	    constexpr auto dist = std::distance(it, std::rend(TypeNameStorage));
	    constexpr auto length = TypeNameStorage.size() - dist;
	    return {TypeNameStorage.data() + dist, length - 1};
    }

    static constexpr std::string_view full_name()
    {
	    // MSVC tested, others are not tested.
        constexpr auto it = std::find(TypeNameStorage.rbegin(), TypeNameStorage.rend(), ' ');
	    if constexpr (it != TypeNameStorage.rend())
	    {
	        constexpr auto dist = std::distance(it, std::rend(TypeNameStorage));
		    return {TypeNameStorage.data() + dist, TypeNameStorage.size() - dist - 1};
	    }

	    return {TypeNameStorage.data(), TypeNameStorage.size() - 1};
    }
};

#define INLINE_COMPILE_TIME_TYPENAME_NON_ENTITY(Type) \
	static std::string_view StaticTypeName() \
	{ \
		return static_type_name<##Type##>::name(); \
	} \
    static std::string_view StaticFullTypeName() \
    { \
		return static_type_name<##Type##>::full_name(); \
    } \
	static HashType StaticTypeHash() \
	{ \
	    return type_hash<##Type##>::value; \
	}

#define INLINE_COMPILE_TIME_TYPENAME(Type) \
	INLINE_COMPILE_TIME_TYPENAME_NON_ENTITY(Type) \
	static bool StaticIsBaseOf(HashType hash) \
	{ \
		return polymorphic_type_hash<##Type##>::is_base_of(hash); \
	} \
	std::string_view GetTypeName() const override { return Type##::StaticFullTypeName(); } \
	std::string_view GetPrettyTypeName() const override { return Type##::StaticTypeName(); } \
	HashType GetTypeHash() const override { return Type##::StaticTypeHash(); } \
	bool IsBaseOf(HashType hash) const override { return Type##::StaticIsBaseOf(hash); }
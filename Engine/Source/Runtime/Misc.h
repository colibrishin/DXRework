#pragma once
#define EMPTY
#define DLLIMPORT __declspec(dllimport)
#define DLLEXPORT __declspec(dllexport)

#define ECLASS(...)
#define EENUM(...)
#define EFUNC(...)
#define EPROPERTY(...)
#define GENERATE_BODY

#include <algorithm>
#include <array>
#include <vector>
#include <cstddef>
#include <string_view>
#include <stdint.h>

//fnv1a 32 and 64 bit hash functions
// key is the data to hash, len is the size of the data (or how much of it to hash against)
// code license: public domain or equivalent
// post: https://notes.underscorediscovery.com/constexpr-fnv1a/

inline const uint32_t hash_32_fnv1a(const void* key, const uint32_t len) {

    const char* data = (char*)key;
    uint32_t hash = 0x811c9dc5;
    uint32_t prime = 0x1000193;

    for(int i = 0; i < len; ++i) {
        uint8_t value = data[i];
        hash = hash ^ value;
        hash *= prime;
    }

    return hash;

} //hash_32_fnv1a

inline const uint64_t hash_64_fnv1a(const void* key, const uint64_t len) {
    
    const char* data = (char*)key;
    uint64_t hash = 0xcbf29ce484222325;
    uint64_t prime = 0x100000001b3;
    
    for(int i = 0; i < len; ++i) {
        uint8_t value = data[i];
        hash = hash ^ value;
        hash *= prime;
    }
    
    return hash;

} //hash_64_fnv1a

// FNV1a c++11 constexpr compile time hash functions, 32 and 64 bit
// str should be a null terminated string literal, value should be left out 
// e.g hash_32_fnv1a_const("example")
// code license: public domain or equivalent
// post: https://notes.underscorediscovery.com/constexpr-fnv1a/

constexpr uint32_t val_32_const = 0x811c9dc5;
constexpr uint32_t prime_32_const = 0x1000193;
constexpr uint64_t val_64_const = 0xcbf29ce484222325;
constexpr uint64_t prime_64_const = 0x100000001b3;

constexpr std::uint32_t hash_32_fnv1a_const(std::string_view str, std::uint32_t value = val_32_const) noexcept
{
	for (auto& c : str) 
	{
		value = (value ^ static_cast<std::uint32_t>(static_cast<uint8_t>(c))) * prime_32_const;
	}
	return value;
}

constexpr std::uint64_t hash_64_fnv1a_const(std::string_view str, std::uint64_t value = val_64_const) noexcept
{
	for (auto& c : str) 
	{
		value = (value ^ static_cast<std::uint64_t>(static_cast<uint8_t>(c))) * prime_64_const;
	}
	return value;
}

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
	bool IsBaseOf(HashType hash) const override{ return Type##::StaticIsBaseOf(hash); }

struct HashType
{
	constexpr bool operator>(const HashType& other) const { return v > other.v; }
	constexpr bool operator>=(const HashType& other) const { return v >= other.v; }
	constexpr bool operator<(const HashType& other) const { return v < other.v; }
	constexpr bool operator<=(const HashType& other) const { return v <= other.v; }
	constexpr bool operator==(const HashType& other) const { return v == other.v; }
	constexpr bool operator!=(const HashType& other) const { return v != other.v; }

	uint32_t v;
#if WITH_DEBUG
	std::string_view name;
#endif
};

template <>
struct std::hash<HashType>
{
	constexpr std::size_t operator()(const HashType& h) const noexcept
	{
		return h.v;
	}
};

template <typename T>
struct type_hash
{
public:
#if WITH_DEBUG
	static constexpr HashType value = HashType{hash_32_fnv1a_const(static_type_name<T>::full_name()), static_type_name<T>::full_name()};
#else
	static constexpr HashType value = HashType{hash_32_fnv1a_const(static_type_name<T>::full_name())};
#endif
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

#define POLYMORPHIC_MANAGER_TYPE_MAP(Type) \
POLYMORPHIC_TYPE_MAP(Engine::Abstracts::Singleton<##Type##>, Engine::Abstracts::SingletonBase) \
POLYMORPHIC_TYPE_MAP(Type, Engine::Abstracts::Singleton<##Type##>)

#define POLYMORPHIC_TYPE_MAP(Type, Base) \
template <>\
struct polymorphic_type_hash<##Type##>\
{\
	static constexpr size_t upcast_count = 1 + polymorphic_type_hash<##Base##>::upcast_count;\
	static constexpr std::array<HashType, upcast_count> upcast_array = []\
	{\
		std::array<HashType, upcast_count> ret{type_hash<##Type##>::value};\
		std::copy_n(polymorphic_type_hash<##Base##>::upcast_array.begin(),  polymorphic_type_hash<##Base##>::upcast_array.size(), ret.data() + 1);\
		std::ranges::sort(ret, std::less<HashType>());\
		if (std::ranges::adjacent_find(ret) != std::ranges::end(ret))\
		{\
			throw std::exception("Duplicated type hash found in upcast array");\
		}\
		return ret;\
	}();\
	static bool is_base_of(const HashType hash) \
	{ \
		if constexpr ((upcast_count * sizeof(HashType)) < (1 << 7)) \
		{ \
			return std::ranges::find(upcast_array, hash) != upcast_array.end(); \
		} \
		return std::ranges::binary_search(upcast_array, hash); \
	} \
};

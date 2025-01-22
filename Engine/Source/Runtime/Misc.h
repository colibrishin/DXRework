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
#include <stdexcept>
#include <exception>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/serialization.hpp>

template<class T, std::size_t... N>
constexpr T bswap_impl(T i, std::index_sequence<N...>)
{
	return ((((i >> (N * CHAR_BIT)) & (T)(unsigned char)(-1)) <<
			 ((sizeof(T) - 1 - N) * CHAR_BIT)) | ...);
}

template<class T, class U = typename std::make_unsigned<T>::type>
constexpr U bswap(T i) {
	return bswap_impl<U>(i, std::make_index_sequence<sizeof(T)>{});
}

#define bswap_32(x) bswap<uint32_t>(x)
#define bswap_64(x) bswap<uint64_t>(x)

#ifdef WORDS_BIGENDIAN
#define uint32_in_expected_order(x) (bswap_32(x))
#define uint64_in_expected_order(x) (bswap_64(x))
#else
#define uint32_in_expected_order(x) (x)
#define uint64_in_expected_order(x) (x)
#endif

#undef PERMUTE3
#define PERMUTE3(a, b, c) do { std::swap(a, b); std::swap(a, c); } while (0)

namespace cityhash
{
	namespace detail
	{
		constexpr uint32_t shift32(uint32_t v, int b)
		{
			return v << (b*8);
		}

		constexpr uint32_t Fetch32(const char s[4])
		{
			return shift32(s[0],0)
				 + shift32(s[1],1)
				 + shift32(s[2],2)
				 + shift32(s[3],3);
		}
  	
		constexpr uint32_t Rotate32(uint32_t val, int shift)
		{
			// Avoid shifting by 32: doing so yields an undefined result.
			return shift == 0 ? val : ((val >> shift) | (val << (32 - shift)));
		}
  	
		// Magic numbers for 32-bit hashing.  Copied from Murmur3.
		static constexpr uint32_t c1 = 0xcc9e2d51;
		static constexpr uint32_t c2 = 0x1b873593;

		constexpr uint32_t Mur(uint32_t a, uint32_t h)
		{
			// Helper from Murmur3 for combining two 32-bit values.
			a *= detail::c1;
			a = Rotate32(a, 17);
			a *= detail::c2;
			h ^= a;
			h = Rotate32(h, 19);
			return h * 5 + 0xe6546b64;
		}

		// A 32-bit to 32-bit integer hash copied from Murmur3.
		constexpr uint32_t fmix(uint32_t h)
		{
			h ^= h >> 16;
			h *= 0x85ebca6b;
			h ^= h >> 13;
			h *= 0xc2b2ae35;
			h ^= h >> 16;
			return h;
		}
  	
		constexpr uint32_t Hash32Len13to24(const char *s, size_t len)
		{
			uint32_t a = Fetch32(s - 4 + (len >> 1));
			uint32_t b = Fetch32(s + 4);
			uint32_t c = Fetch32(s + len - 8);
			uint32_t d = Fetch32(s + (len >> 1));
			uint32_t e = Fetch32(s);
			uint32_t f = Fetch32(s + len - 4);
			uint32_t h = static_cast<uint32_t>(len);

			return fmix(Mur(f, Mur(e, Mur(d, Mur(c, Mur(b, Mur(a, h)))))));
		}

		constexpr uint32_t Hash32Len0to4(const char *s, size_t len)
		{
			uint32_t b = 0;
			uint32_t c = 9;
			for (size_t i = 0; i < len; i++) {
				signed char v = static_cast<signed char>(s[i]);
				b = b * detail::c1 + static_cast<uint32_t>(v);
				c ^= b;
			}
			return fmix(Mur(b, Mur(static_cast<uint32_t>(len), c)));
		}

		constexpr uint32_t Hash32Len5to12(const char *s, size_t len)
		{
			uint32_t a = static_cast<uint32_t>(len), b = a * 5, c = 9, d = b;
			a += Fetch32(s);
			b += Fetch32(s + len - 4);
			c += Fetch32(s + ((len >> 1) & 4));
			return fmix(Mur(c, Mur(b, Mur(a, d))));
		}
	}

	constexpr uint32_t CityHash32(const char *s, size_t len)
	{
		if (len <= 24) {
			return len <= 12 ?
				(len <= 4 ? detail::Hash32Len0to4(s, len) : detail::Hash32Len5to12(s, len)) :
				detail::Hash32Len13to24(s, len);
		}

		// len > 24
		uint32_t h = static_cast<uint32_t>(len), g = detail::c1 * h, f = g;
		uint32_t a0 = detail::Rotate32(detail::Fetch32(s + len - 4) * detail::c1, 17) * detail::c2;
		uint32_t a1 = detail::Rotate32(detail::Fetch32(s + len - 8) * detail::c1, 17) * detail::c2;
		uint32_t a2 = detail::Rotate32(detail::Fetch32(s + len - 16) * detail::c1, 17) * detail::c2;
		uint32_t a3 = detail::Rotate32(detail::Fetch32(s + len - 12) * detail::c1, 17) * detail::c2;
		uint32_t a4 = detail::Rotate32(detail::Fetch32(s + len - 20) * detail::c1, 17) * detail::c2;
		h ^= a0;
		h = detail::Rotate32(h, 19);
		h = h * 5 + 0xe6546b64;
		h ^= a2;
		h = detail::Rotate32(h, 19);
		h = h * 5 + 0xe6546b64;
		g ^= a1;
		g = detail::Rotate32(g, 19);
		g = g * 5 + 0xe6546b64;
		g ^= a3;
		g = detail::Rotate32(g, 19);
		g = g * 5 + 0xe6546b64;
		f += a4;
		f = detail::Rotate32(f, 19);
		f = f * 5 + 0xe6546b64;
		size_t iters = (len - 1) / 20;
		do {
			uint32_t a0 = detail::Rotate32(detail::Fetch32(s) * detail::c1, 17) * detail::c2;
			uint32_t a1 = detail::Fetch32(s + 4);
			uint32_t a2 = detail::Rotate32(detail::Fetch32(s + 8) * detail::c1, 17) * detail::c2;
			uint32_t a3 = detail::Rotate32(detail::Fetch32(s + 12) * detail::c1, 17) * detail::c2;
			uint32_t a4 = detail::Fetch32(s + 16);
			h ^= a0;
			h = detail::Rotate32(h, 18);
			h = h * 5 + 0xe6546b64;
			f += a1;
			f = detail::Rotate32(f, 19);
			f = f * detail::c1;
			g += a2;
			g = detail::Rotate32(g, 18);
			g = g * 5 + 0xe6546b64;
			h ^= a3 + a1;
			h = detail::Rotate32(h, 19);
			h = h * 5 + 0xe6546b64;
			g ^= a4;
			g = bswap_32(g) * 5;
			h += a4 * 5;
			h = bswap_32(h);
			f += a0;
			PERMUTE3(f, h, g);
			s += 20;
		} while (--iters != 0);
		g = detail::Rotate32(g, 11) * detail::c1;
		g = detail::Rotate32(g, 17) * detail::c1;
		f = detail::Rotate32(f, 11) * detail::c1;
		f = detail::Rotate32(f, 17) * detail::c1;
		h = detail::Rotate32(h + g, 19);
		h = h * 5 + 0xe6546b64;
		h = detail::Rotate32(h, 17) * detail::c1;
		h = detail::Rotate32(h + f, 19);
		h = h * 5 + 0xe6546b64;
		h = detail::Rotate32(h, 17) * detail::c1;
		return h;
	}
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
	    return &type_hash<##Type##>::value; \
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

struct HashTypeImpl
{
	constexpr bool operator>(const HashTypeImpl& other) const { return v > other.v; }
	constexpr bool operator>=(const HashTypeImpl& other) const { return v >= other.v; }
	constexpr bool operator<(const HashTypeImpl& other) const { return v < other.v; }
	constexpr bool operator<=(const HashTypeImpl& other) const { return v <= other.v; }
	constexpr bool operator==(const HashTypeImpl& other) const { return Equal(other); }
	constexpr bool operator!=(const HashTypeImpl& other) const { return !Equal(other); }

	constexpr virtual bool Equal(const HashTypeImpl& other) const
	{
		return v == other.v;
	}
	virtual const HashTypeImpl* Fetch() const
	{
		throw std::runtime_error("Cannot fetch a hash from a base class.");
		return nullptr;
	}

	constexpr HashTypeImpl() = default;
	constexpr HashTypeImpl(uint32_t value) : v(value) {}

	uint32_t v;
	
private:
	friend class boost::serialization::access;
	friend struct std::hash<HashTypeImpl>;
	
	template <typename Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& v;
	}
};

using HashTypeValue = const HashTypeImpl;
using HashType = const HashTypeValue*;

template <typename T> struct type_hash;

template <typename T>
struct HashTypeT : public HashTypeImpl
{
	constexpr bool operator>(const HashTypeT& other) const { return v > other.v; }
	constexpr bool operator>=(const HashTypeT& other) const { return v >= other.v; }
	constexpr bool operator<(const HashTypeT& other) const { return v < other.v; }
	constexpr bool operator<=(const HashTypeT& other) const { return v <= other.v; }
	constexpr bool operator==(const HashTypeT& other) const { return Equal(other); }
	constexpr bool operator!=(const HashTypeT& other) const { return !Equal(other); }

	constexpr bool Equal(const HashTypeImpl& other) const override
	{
		return HashTypeImpl::Equal(other) && this == &other;
	}

	HashType Fetch() const override
	{
		return &type_hash<T>::value;
	}
	
	constexpr HashTypeT() :
	HashTypeImpl(cityhash::CityHash32(static_type_name<T>::full_name().data(), static_type_name<T>::full_name().size())) {}

private:
	friend class boost::serialization::access;
	
	template <typename Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& boost::serialization::base_object<HashTypeImpl>(*this);
	}
};

template <typename Archive>
void serialize(Archive& ar, HashType& x, const unsigned int version)
{
	ar& x;
	if (Archive::is_loading::value)
	{
		if (x != nullptr)
		{
			HashType* runtime_fetched = x->Fetch();
			x = runtime_fetched;
		}
	}
}

template <>
struct std::hash<HashTypeImpl>
{
	constexpr std::size_t operator()(const HashTypeImpl& h) const noexcept
	{
		return h.v;
	}
};

template <typename T>
struct type_hash
{
public:
	static constexpr HashTypeT<T> value{};
};

template <size_t Count>
using HashArray = std::array<HashType, Count>;

template <typename T>
struct polymorphic_type_hash
{
	static constexpr size_t upcast_count = 0;
	static constexpr HashArray<upcast_count> upcast_array {};

	static bool is_base_of(const HashType base)
	{
		return false;
	}
};

template <>
struct polymorphic_type_hash<void>
{
	static constexpr size_t upcast_count = 1;
	static constexpr HashArray<upcast_count> upcast_array {&type_hash<void>::value};
};

#define POLYMORPHIC_MANAGER_TYPE_MAP(Type) \
POLYMORPHIC_TYPE_MAP(Engine::Abstracts::Singleton<##Type##>, Engine::Abstracts::SingletonBase) \
POLYMORPHIC_TYPE_MAP(Type, Engine::Abstracts::Singleton<##Type##>)

#define POLYMORPHIC_TYPE_MAP(Type, Base) \
template <>\
struct polymorphic_type_hash<##Type##>\
{\
	static constexpr size_t upcast_count = 1 + polymorphic_type_hash<##Base##>::upcast_count;\
	static constexpr auto upcast_array = []\
	{\
		HashArray<upcast_count> ret{&type_hash<##Type##>::value};\
		std::copy_n(polymorphic_type_hash<##Base##>::upcast_array.begin(),  polymorphic_type_hash<##Base##>::upcast_array.size(), ret.data() + 1);\
		std::ranges::sort(ret, [](const auto lhs, const auto rhs) {return *lhs < *rhs;});\
		return ret;\
	}();\
	static bool is_base_of(const HashType hash)\
	{\
		if constexpr ((upcast_count * sizeof(HashTypeValue)) < (1 << 7))\
		{\
			return std::ranges::find_if(upcast_array, [&hash](const auto other){return hash->Equal(*other);}) != upcast_array.end();\
		}\
		return std::ranges::binary_search(upcast_array, hash, [](const auto lhs, const auto rhs){return *lhs < *rhs;});\
	}\
};

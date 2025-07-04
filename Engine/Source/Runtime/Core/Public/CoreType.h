#pragma once
#define EMPTY
#ifndef DLLIMPORT
#define DLLIMPORT __declspec( dllimport )
#endif

#ifndef DLLEXPORT
#define DLLEXPORT __declspec( dllexport )
#endif

#define ECLASS(...)
#define EENUM(...)
#define EFUNC(...)
#define EPROPERTY(...)
#define GENERATE_BODY

#define STRINGIFY(X) STRINGIFY_IMPL(X)
#define STRINGIFY_IMPL(X) #X

#include <algorithm>
#include <array>
#include <vector>
#include <cstddef>
#include <magic_enum.hpp>
#include <string_view>
#include <stdint.h>
#include <stdexcept>
#include <unordered_set>
#include <map>
#include <memory>
#include <mutex>
#include <type_traits>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/archive/detail/iserializer.hpp>
#include <boost/functional/hash.hpp>
#include <boost/serialization/access.hpp>
#include <boost/pool/pool_alloc.hpp>

#include <magic_enum.hpp>

template <typename Enum>
constexpr auto CStrEnumStrings()
{
    constexpr auto                            enum_val = magic_enum::enum_names<Enum>();
    std::array<const char *, enum_val.size()> ret{};
    for ( size_t i = 0; i < enum_val.size(); ++i )
    {
        ret[ i ] = enum_val[ i ].data();
    }
    return ret;
}

template <typename Enum>
Enum RecastNonlinearEnum( const auto &cstr_array, size_t value )
{
    if ( const auto format_validity = magic_enum::enum_cast<Enum>( cstr_array[ value ] ); format_validity.has_value() )
    {
        return format_validity.value();
    }

    return static_cast<Enum>( 0 );
}

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

template <typename T>
struct is_serializable : std::false_type {};

template <typename T>
struct is_internal : std::false_type {};

template <typename T>
constexpr bool is_serializable_v = std::is_base_of_v<std::true_type, is_serializable<T>>;

template <typename T>
constexpr bool is_internal_v = std::is_base_of_v<std::true_type, is_internal<T>>;

template <typename T, typename U, typename = void>
struct is_val_cont : std::false_type
{};

template <typename T, typename U>
struct is_val_cont<T, U, std::void_t<decltype( typename T::template value_type{} == U{} )>> : std::true_type
{};

template <typename T, typename U>
constexpr bool is_val_cont_v = is_val_cont<T, U>::value;

template <typename Cont, typename Val, typename = void>
struct is_key_val_cont : std::false_type {};

template <typename Cont, typename Val>
struct is_key_val_cont<Cont,
                       Val,
                       std::void_t<decltype( typename Cont::template value_type::second_type{} == Val{} ),
                                   std::is_same<typename Cont::template value_type::template second_type, Val>>>
    : std::true_type
{};

template <typename T, typename U>
constexpr bool is_key_val_cont_v = is_key_val_cont<T, U>::value;

#define bswap_32(x) bswap<uint32_t>(x)
#define bswap_64(x) bswap<uint64_t>(x)

#ifdef WORDS_BIGENDIAN
#define uint32_in_expected_order(x) (bswap_32(x))
#define uint64_in_expected_order(x) (bswap_64(x))
#else
#define uint32_in_expected_order(x) (x)
#define uint64_in_expected_order(x) (x)
#endif	

#if !defined(LIKELY)
#if HAVE_BUILTIN_EXPECT
#define LIKELY(x) (__builtin_expect(!!(x), 1))
#else
#define LIKELY(x) (x)
#endif
#endif

#undef PERMUTE3
#define PERMUTE3(a, b, c) do { std::swap(a, b); std::swap(a, c); } while (0)

struct simple_gc_deleter_impl
{
	simple_gc_deleter_impl(const void* ptr) : ptr(ptr) {}
	const void* ptr;

	virtual ~simple_gc_deleter_impl() = default;
	virtual void call_delete() const = 0;
};

template <typename T>
struct simple_gc_deleter : simple_gc_deleter_impl 
{
	simple_gc_deleter(const T* ptr) : simple_gc_deleter_impl(static_cast<const void*>(ptr)) {}

	~simple_gc_deleter() override = default;

	void call_delete() const override
	{
		delete static_cast<const T*>(ptr);
	}
};

class byte_stream
{
public:
    explicit byte_stream()
        : m_first_(nullptr),
          m_allocated_size_(0),
          m_used_size_(0) {}

    ~byte_stream()
    {
        delete[] m_first_;
    }

	byte_stream(byte_stream&& other) noexcept
    {
        operator=( std::move( other ) );
    }

	byte_stream& operator=(byte_stream&& other) noexcept
    {
        m_first_          = std::move( other.m_first_ );
        m_allocated_size_ = std::move( m_allocated_size_ );
        m_used_size_      = std::move( m_used_size_ );
        return *this;
    }

    void reset()
    {
        m_used_size_ = 0;
    }

    [[nodiscard]] void* data() const
    {
        return m_first_;
    }

    [[nodiscard]] size_t size() const
    {
        return m_used_size_;
    }
    
    void push_back(const void* src, const size_t src_size)
    {
        if (m_used_size_ + src_size >= m_allocated_size_)
        {
            if (m_allocated_size_ == 0)
            {
                m_allocated_size_ = src_size * 2;
            }
            else
            {
                m_allocated_size_ = m_allocated_size_ * 2;
            }
            
            auto new_alloc = new unsigned char[m_allocated_size_];
            if (m_first_)
            {
                std::memcpy(new_alloc, m_first_, m_used_size_);
                delete[] m_first_;
                m_first_ = nullptr;
            }
            m_first_ = new_alloc;
        }

        if (m_first_ != nullptr) {
            std::memcpy(m_first_ + m_used_size_, src, src_size);
        }
        m_used_size_ += src_size;
    }

private:
    unsigned char* m_first_;
    
    size_t m_allocated_size_;
    size_t m_used_size_;
};

struct simple_gc_collector 
{
private:
	inline static std::unordered_set<const void*> s_collected = {};
	inline static std::unordered_set<std::unique_ptr<simple_gc_deleter_impl>> s_deleters = {};
	inline static std::mutex s_lock;

	static void Release()
	{
		std::lock_guard l(s_lock);
		for (auto& deleter : s_deleters) 
		{
			deleter->call_delete();
		}
		s_collected.clear();
		s_deleters.clear();
	}
	friend struct simple_gc_scope;
public:
	template <typename T>
	static void Add(const T* ptr)
	{
		std::lock_guard l(s_lock);
		if (!s_collected.contains(ptr)) 
		{
			s_collected.insert(ptr);
			s_deleters.insert(std::make_unique<simple_gc_deleter<T>>(ptr));
		}
	}
};

struct simple_gc_scope
{
	~simple_gc_scope()
	{
		simple_gc_collector::Release();
	}
};

template <typename T>
T ZeroSet()
{
	T new_t{};
	// cannot be defined as constexpr due to the reinterpret cast.
	std::fill_n(reinterpret_cast<char*>(&new_t), sizeof(T), 0);
	return new_t;
}

namespace crc32
{
	/// merge two CRC32 such that result = crc32(dataB, lengthB, crc32(dataA, lengthA))
	constexpr uint64_t crc32_combine(uint32_t crcA, uint64_t crcB, size_t lengthB)
	{
		/// zlib's CRC32 polynomial
		const uint32_t Polynomial = 0xEDB88320;

		// based on Mark Adler's crc_combine from
		// https://github.com/madler/pigz/blob/master/pigz.c

		// main idea:
		// - if you have two equally-sized blocks A and B,
		//   then you can create a block C = A ^ B
		//   which has the property crc(C) = crc(A) ^ crc(B)
		// - if you append length(B) zeros to A and call it A' (think of it as AAAA000)
		//   and   prepend length(A) zeros to B and call it B' (think of it as 0000BBB)
		//   then exists a C' = A' ^ B'
		// - remember: if you XOR someting with zero, it remains unchanged: X ^ 0 = X
		// - that means C' = A concat B so that crc(A concat B) = crc(C') = crc(A') ^ crc(B')
		// - the trick is to compute crc(A') based on crc(A)
		//                       and crc(B') based on crc(B)
		// - since B' starts with many zeros, the crc of those initial zeros is still zero
		// - that means crc(B') = crc(B)
		// - unfortunately the trailing zeros of A' change the crc, so usually crc(A') != crc(A)
		// - the following code is a fast algorithm to compute crc(A')
		// - starting with crc(A) and appending length(B) zeros, needing just log2(length(B)) iterations
		// - the details are explained by the original author at
		//   https://stackoverflow.com/questions/23122312/crc-calculation-of-a-mostly-static-data-stream/23126768
		//
		// notes:
		// - I squeezed everything into one function to keep global namespace clean (original code two helper functions)
		// - most original comments are still in place, I added comments where these helper functions where made inline code
		// - performance-wise there isn't any differenze to the original zlib/pigz code

		// degenerated case
		if (lengthB == 0)
			return crcA;

		/// CRC32 => 32 bits
		const uint32_t CrcBits = 32;

		uint32_t odd[CrcBits]; // odd-power-of-two  zeros operator
		uint32_t even[CrcBits]; // even-power-of-two zeros operator

		// put operator for one zero bit in odd
		odd[0] = Polynomial;    // CRC-32 polynomial
		for (int i = 1; i < (int)CrcBits; i++)
			odd[i] = 1 << (i - 1);

		// put operator for two zero bits in even
		// same as gf2_matrix_square(even, odd);
		for (int i = 0; i < (int)CrcBits; i++)
		{
			uint32_t vec = odd[i];
			even[i] = 0;
			for (int j = 0; vec != 0; j++, vec >>= 1)
				if (vec & 1)
					even[i] ^= odd[j];
		}
		// put operator for four zero bits in odd
		// same as gf2_matrix_square(odd, even);
		for (int i = 0; i < (int)CrcBits; i++)
		{
			uint32_t vec = even[i];
			odd[i] = 0;
			for (int j = 0; vec != 0; j++, vec >>= 1)
				if (vec & 1)
					odd[i] ^= even[j];
		}

		// the following loop becomes much shorter if I keep swapping even and odd
		uint32_t* a = even;
		uint32_t* b = odd;
		// apply secondLength zeros to firstCrc32
		for (; lengthB > 0; lengthB >>= 1)
		{
			// same as gf2_matrix_square(a, b);
			for (int i = 0; i < (int)CrcBits; i++)
			{
				uint32_t vec = b[i];
				a[i] = 0;
				for (int j = 0; vec != 0; j++, vec >>= 1)
					if (vec & 1)
						a[i] ^= b[j];
			}

			// apply zeros operator for this bit
			if (lengthB & 1)
			{
				// same as firstCrc32 = gf2_matrix_times(a, firstCrc32);
				uint32_t sum = 0;
				for (int i = 0; crcA != 0; i++, crcA >>= 1)
					if (crcA & 1)
						sum ^= a[i];
				crcA = sum;
			}

			// switch even and odd
			uint32_t* t = a; a = b; b = t;
		}

		uint64_t combined = crcA ^ crcB;

		// return combined crc
		return static_cast<uint32_t>(combined);
	}
}

namespace boost_constexpr
{
	namespace hash_detail
	{
		template<std::size_t Bits> struct hash_mix_impl;

		// hash_mix for 64 bit size_t
		//
		// The general "xmxmx" form of state of the art 64 bit mixers originates
		// from Murmur3 by Austin Appleby, which uses the following function as
		// its "final mix":
		//
		//	k ^= k >> 33;
		//	k *= 0xff51afd7ed558ccd;
		//	k ^= k >> 33;
		//	k *= 0xc4ceb9fe1a85ec53;
		//	k ^= k >> 33;
		//
		// (https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp)
		//
		// It has subsequently been improved multiple times by different authors
		// by changing the constants. The most well known improvement is the
		// so-called "variant 13" function by David Stafford:
		//
		//	k ^= k >> 30;
		//	k *= 0xbf58476d1ce4e5b9;
		//	k ^= k >> 27;
		//	k *= 0x94d049bb133111eb;
		//	k ^= k >> 31;
		//
		// (https://zimbry.blogspot.com/2011/09/better-bit-mixing-improving-on.html)
		//
		// This mixing function is used in the splitmix64 RNG:
		// http://xorshift.di.unimi.it/splitmix64.c
		//
		// We use Jon Maiga's implementation from
		// http://jonkagstrom.com/mx3/mx3_rev2.html
		//
		// 	x ^= x >> 32;
		//	x *= 0xe9846af9b1a615d;
		//	x ^= x >> 32;
		//	x *= 0xe9846af9b1a615d;
		//	x ^= x >> 28;
		//
		// An equally good alternative is Pelle Evensen's Moremur:
		//
		//	x ^= x >> 27;
		//	x *= 0x3C79AC492BA7B653;
		//	x ^= x >> 33;
		//	x *= 0x1C69B3F74AC4AE35;
		//	x ^= x >> 27;
		//
		// (https://mostlymangling.blogspot.com/2019/12/stronger-better-morer-moremur-better.html)

		template<> struct hash_mix_impl<64>
		{
			constexpr static std::uint64_t fn(std::uint64_t x)
			{
				std::uint64_t const m = 0xe9846af9b1a615d;

				x ^= x >> 32;
				x *= m;
				x ^= x >> 32;
				x *= m;
				x ^= x >> 28;

				return x;
			}
		};

		// hash_mix for 32 bit size_t
		//
		// We use the "best xmxmx" implementation from
		// https://github.com/skeeto/hash-prospector/issues/19

		template<> struct hash_mix_impl<32>
		{
			constexpr static std::uint32_t fn(std::uint32_t x)
			{
				std::uint32_t const m1 = 0x21f0aaad;
				std::uint32_t const m2 = 0x735a2d97;

				x ^= x >> 16;
				x *= m1;
				x ^= x >> 15;
				x *= m2;
				x ^= x >> 15;

				return x;
			}
		};

		inline std::size_t hash_mix(std::size_t v)
		{
			return hash_mix_impl<sizeof(std::size_t) * CHAR_BIT>::fn(v);
		}

	}

	template <class T>
	constexpr void hash_combine(std::size_t& seed, T const& v)
	{
		seed = boost::hash_detail::hash_mix(seed + 0x9e3779b9 + v);
	}
}

namespace cityhash
{
	struct cityhash256
	{
		constexpr bool operator<(const cityhash256& other) const noexcept
		{
			for (int i = 0; i < 4; ++i)
			{
				if (v[i] == other.v[i])
				{
					continue;
				}
				return v[i] < other.v[i];
			}

			return false;
		}

		constexpr bool operator<=(const cityhash256& other) const noexcept
		{
			for (int i = 0; i < 4; ++i)
			{
				if (v[i] > other.v[i])
				{
					return false;
				}
			}

			return true;
		}

		constexpr bool operator>(const cityhash256& other) const noexcept
		{
			return !(*this < other);
		}

		constexpr bool operator>=(const cityhash256& other) const noexcept
		{
			return !(*this <= other);
		}

		constexpr bool operator==(const cityhash256& other) const noexcept
		{
			bool flag = true;
			for (int i = 0; i < 4; ++i) {
				flag &= (v[i] == other.v[i]);
			}

			return flag;
		}

		constexpr bool operator!=(const cityhash256& other) const noexcept
		{
			return !(*this == other);
		}

		constexpr std::size_t hash() const noexcept
		{
			size_t seed = v[0];
			boost_constexpr::hash_combine(seed, v[1]);
			boost_constexpr::hash_combine(seed, v[2]);
			boost_constexpr::hash_combine(seed, v[3]);
			return seed;
		}

		constexpr cityhash256() : v{} {}

		uint64_t v[4];

	private:
		friend class boost::serialization::access;

		template <typename Archive>
		void serialize(Archive& ar, const unsigned int /*version*/) 
		{
			ar& v;
		}
	};

	namespace detail
	{
		constexpr uint64_t shift64(uint64_t v, int b)
		{
			return v << (b * 8);
		}

		constexpr uint32_t shift32(uint32_t v, int b)
		{
			return v << (b * 8);
		}

		constexpr uint32_t Fetch32(const char s[4])
		{
			return shift32(s[0], 0)
				+ shift32(s[1], 1)
				+ shift32(s[2], 2)
				+ shift32(s[3], 3);
		}

		constexpr uint64_t Fetch64(const char s[8])
		{
			return shift64(s[0], 0)
				+ shift64(s[1], 1)
				+ shift64(s[2], 2)
				+ shift64(s[3], 3)
				+ shift64(s[4], 4)
				+ shift64(s[5], 5)
				+ shift64(s[6], 6)
				+ shift64(s[7], 7);
		}

		constexpr uint32_t Rotate32(uint32_t val, int shift)
		{
			// Avoid shifting by 32: doing so yields an undefined result.
			return shift == 0 ? val : ((val >> shift) | (val << (32 - shift)));
		}

		constexpr uint64_t Rotate64(uint64_t val, int shift)
		{
			// Avoid shifting by 32: doing so yields an undefined result.
			return shift == 0 ? val : ((val >> shift) | (val << (64 - shift)));
		}

		// Magic numbers for 32-bit hashing.  Copied from Murmur3.
		static constexpr uint32_t c1 = 0xcc9e2d51;
		static constexpr uint32_t c2 = 0x1b873593;

		// Some primes between 2^63 and 2^64 for various uses.
		static constexpr uint64_t k0 = 0xc3a5c85c97cb3127ULL;
		static constexpr uint64_t k1 = 0xb492b66fbe98f273ULL;
		static constexpr uint64_t k2 = 0x9ae16a3b2f90404fULL;

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

		constexpr uint32_t Hash32Len13to24(const char* s, size_t len)
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

		constexpr uint32_t Hash32Len0to4(const char* s, size_t len)
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

		constexpr uint32_t Hash32Len5to12(const char* s, size_t len)
		{
			uint32_t a = static_cast<uint32_t>(len), b = a * 5, c = 9, d = b;
			a += Fetch32(s);
			b += Fetch32(s + len - 4);
			c += Fetch32(s + ((len >> 1) & 4));
			return fmix(Mur(c, Mur(b, Mur(a, d))));
		}

		typedef std::pair<uint64_t, uint64_t> uint128;

		constexpr uint64_t Uint128Low64(const uint128& x) { return x.first; }
		constexpr uint64_t Uint128High64(const uint128& x) { return x.second; }

		constexpr uint64_t Hash128to64(const uint128& x)
		{
			// Murmur-inspired hashing.
			const uint64_t kMul = 0x9ddfea08eb382d69ULL;
			uint64_t a = (Uint128Low64(x) ^ Uint128High64(x)) * kMul;
			a ^= (a >> 47);
			uint64_t b = (Uint128High64(x) ^ a) * kMul;
			b ^= (b >> 47);
			b *= kMul;
			return b;
		}

		constexpr uint64_t HashLen16(uint64_t u, uint64_t v)
		{
			return Hash128to64(uint128(u, v));
		}

		constexpr uint64_t HashLen16(uint64_t u, uint64_t v, uint64_t mul)
		{
			// Murmur-inspired hashing.
			uint64_t a = (u ^ v) * mul;
			a ^= (a >> 47);
			uint64_t b = (v ^ a) * mul;
			b ^= (b >> 47);
			b *= mul;
			return b;
		}

		constexpr uint64_t ShiftMix(uint64_t val) 
		{
			return val ^ (val >> 47);
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
			a0 = detail::Rotate32(detail::Fetch32(s) * detail::c1, 17) * detail::c2;
			a1 = detail::Fetch32(s + 4);
			a2 = detail::Rotate32(detail::Fetch32(s + 8) * detail::c1, 17) * detail::c2;
			a3 = detail::Rotate32(detail::Fetch32(s + 12) * detail::c1, 17) * detail::c2;
			a4 = detail::Fetch32(s + 16);
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

	// Requires len >= 240.
	constexpr void CityHashCrc256Long(const char* s, size_t len, uint32_t seed, uint64_t* result) 
	{
		uint64_t a = detail::Fetch64(s + 56) + detail::k0;
		uint64_t b = detail::Fetch64(s + 96) + detail::k0;
		uint64_t c = result[0] = detail::HashLen16(b, len);
		uint64_t d = result[1] = detail::Fetch64(s + 120) * detail::k0 + len;
		uint64_t e = detail::Fetch64(s + 184) + seed;
		uint64_t f = 0;
		uint64_t g = 0;
		uint64_t h = c + d;
		uint64_t x = seed;
		uint64_t y = 0;
		uint64_t z = 0;

		// 240 bytes of input per iter.
		size_t iters = len / 240;
		len -= iters * 240;
		do {
#undef CHUNK
#define CHUNK(r)                                \
    PERMUTE3(x, z, y);                          \
    b += detail::Fetch64(s);                    \
    c += detail::Fetch64(s + 8);                \
    d += detail::Fetch64(s + 16);               \
    e += detail::Fetch64(s + 24);               \
    f += detail::Fetch64(s + 32);               \
    a += b;                                     \
    h += f;                                     \
    b += c;                                     \
    f += d;                                     \
    g += e;                                     \
    e += z;                                     \
    g += x;                                     \
    z = crc32::crc32_combine(z, b + g, 64);     \
    y = crc32::crc32_combine(y, e + h, 64);     \
    x = crc32::crc32_combine(x, f + a, 64);     \
    e = detail::Rotate64(e, r);                 \
    c += e;                                     \
    s += 40

			CHUNK(0); PERMUTE3(a, h, c);
			CHUNK(33); PERMUTE3(a, h, f);
			CHUNK(0); PERMUTE3(b, h, f);
			CHUNK(42); PERMUTE3(b, h, d);
			CHUNK(0); PERMUTE3(b, h, e);
			CHUNK(33); PERMUTE3(a, h, e);
		} while (--iters > 0);

		while (len >= 40) {
			CHUNK(29);
			e ^= detail::Rotate64(a, 20);
			h += detail::Rotate64(b, 30);
			g ^= detail::Rotate64(c, 40);
			f += detail::Rotate64(d, 34);
			PERMUTE3(c, h, g);
			len -= 40;
		}
		if (len > 0) {
			s = s + len - 40;
			CHUNK(33);
			e ^= detail::Rotate64(a, 43);
			h += detail::Rotate64(b, 42);
			g ^= detail::Rotate64(c, 41);
			f += detail::Rotate64(d, 40);
		}
		result[0] ^= h;
		result[1] ^= g;
		g += h;
		a = detail::HashLen16(a, g + z);
		x += y << 32;
		b += x;
		c = detail::HashLen16(c, z) + h;
		d = detail::HashLen16(d, e + result[0]);
		g += e;
		h += detail::HashLen16(x, f);
		e = detail::HashLen16(a, d) + g;
		z = detail::HashLen16(b, c) + a;
		y = detail::HashLen16(g, h) + c;
		result[0] = e + z + y + x;
		a = detail::ShiftMix((a + y) * detail::k0) * detail::k0 + b;
		result[1] += a + result[0];
		a = detail::ShiftMix(a * detail::k0) * detail::k0 + c;
		result[2] = a + result[1];
		a = detail::ShiftMix((a + e) * detail::k0) * detail::k0;
		result[3] = a + result[2];
	}

	// Requires len < 240.
	constexpr void CityHashCrc256Short(const char* s, size_t len, uint64_t* result) {
		char buf[240]{};
		std::copy_n(s, len, std::begin(buf));
		CityHashCrc256Long(buf, 240, ~static_cast<uint32_t>(len), result);
	}

	constexpr void CityHashCrc256(const char* s, size_t len, uint64_t* result) {
		if (LIKELY(len >= 240)) {
			CityHashCrc256Long(s, len, 0, result);
		}
		else {
			CityHashCrc256Short(s, len, result);
		}
	}

	constexpr cityhash256 CityHashCrc256_s(const char* s, size_t len) 
	{
		cityhash256 retval{};
		
		if (LIKELY(len >= 240)) {
			CityHashCrc256Long(s, len, 0, retval.v);
		}
		else {
			CityHashCrc256Short(s, len, retval.v);
		}

		return retval;
	}
#undef CHUNK
}

BOOST_CLASS_EXPORT_KEY(cityhash::cityhash256)

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
			constexpr auto space_sep = std::find(TypeNameStorage.rbegin(), TypeNameStorage.rend(), ' ');
			constexpr auto dist = std::distance(space_sep, std::rend(TypeNameStorage));
			constexpr auto length = TypeNameStorage.size() - dist;
		    return {TypeNameStorage.data() + dist, length - 1};
	    }

#pragma warning(disable : 4702)
	    constexpr auto dist = std::distance(it, std::rend(TypeNameStorage));
	    constexpr auto length = TypeNameStorage.size() - dist;
	    return {TypeNameStorage.data() + dist, length - 1};
#pragma warning(default: 4702)
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

#pragma warning( disable : 4702 )
    	return {TypeNameStorage.data(), TypeNameStorage.size() - 1};
#pragma warning( default : 4702 ) 
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
	static bool StaticIsDerivedOf(HashType base) \
	{ \
		return polymorphic_type_hash<##Type##>::is_derived_of(base); \
	} \
	virtual std::string_view GetTypeName() const { return Type##::StaticFullTypeName(); } \
	virtual std::string_view GetPrettyTypeName() const { return Type##::StaticTypeName(); } \
	virtual HashType GetTypeHash() const { return Type##::StaticTypeHash(); } \
	virtual bool IsDerivedOf(HashType base) const { return Type##::StaticIsDerivedOf(base); } \
	virtual bool IsBaseOf(HashType derived) const { return derived->IsDerivedOf(Type##::StaticTypeHash()); }

struct ENGINE_CORE_API HashTypeImpl
{
	virtual        ~HashTypeImpl() = default;
	constexpr bool operator>(const HashTypeImpl& other) const { return v > other.v; }
	constexpr bool operator>=(const HashTypeImpl& other) const { return v >= other.v; }
	constexpr bool operator<(const HashTypeImpl& other) const { return v < other.v; }
	constexpr bool operator<=(const HashTypeImpl& other) const { return v <= other.v; }
	constexpr bool operator==(const HashTypeImpl& other) const { return Equal(other); }
	constexpr bool operator!=(const HashTypeImpl& other) const { return !Equal(other); }

	[[nodiscard]] constexpr virtual bool Equal(const HashTypeImpl& other) const
	{
		return v == other.v;
	}
	[[nodiscard]] virtual const HashTypeImpl* Fetch() const
	{
		throw std::runtime_error("Not Implemented");
	}
	[[nodiscard]] virtual bool IsDerivedOf(const HashTypeImpl* /*base*/) const 
	{
		throw std::runtime_error("Not Implemented");
	}
	[[nodiscard]] virtual bool IsBaseOf(const HashTypeImpl* /*derived*/) const
	{
		throw std::runtime_error("Not Implemented");
	}
	[[nodiscard]] virtual bool IsSerializable() const 
	{
		throw std::runtime_error("Not Implemented");
	}
	[[nodiscard]] virtual bool IsInternal() const
	{
		throw std::runtime_error("Not Implemented");
	}
	[[nodiscard]] virtual std::string_view GetTypeName() const
	{
		throw std::runtime_error("Not Implemented");
	}

	constexpr HashTypeImpl() = default;
	constexpr HashTypeImpl(const cityhash::cityhash256& value) : v(value) {}

	cityhash::cityhash256 v;
	
private:
	friend class boost::serialization::access;
	friend struct std::hash<HashTypeImpl>;
	
	template <typename Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& v;
	}
};

BOOST_CLASS_EXPORT_KEY(HashTypeImpl)

using HashTypeValue = const HashTypeImpl;
using HashType = const HashTypeValue*;

template <typename T> struct type_hash;

template <size_t Count>
using HashArray = std::array<HashType, Count>;

template <typename T>
struct polymorphic_type_hash
{
	static constexpr size_t upcast_count = 0;
	static constexpr HashArray<upcast_count> upcast_array{};

	constexpr static bool is_derived_of(const HashType /*base*/)
	{
		return false;
	}
};

template <typename T>
struct HashTypeT : HashTypeImpl
{
	constexpr bool operator>(const HashTypeT& other) const { return v > other.v; }
	constexpr bool operator>=(const HashTypeT& other) const { return v >= other.v; }
	constexpr bool operator<(const HashTypeT& other) const { return v < other.v; }
	constexpr bool operator<=(const HashTypeT& other) const { return v <= other.v; }
	constexpr bool operator==(const HashTypeT& other) const { return Equal(other); }
	constexpr bool operator!=(const HashTypeT& other) const { return !Equal(other); }

	[[nodiscard]] constexpr bool Equal(const HashTypeImpl& other) const override
	{
		return HashTypeImpl::Equal(other) && this == &other;
	}
	[[nodiscard]] HashType Fetch() const override
	{
		return &type_hash<T>::value;
	}
	bool IsDerivedOf(const HashTypeImpl* base) const override
	{
		return polymorphic_type_hash<T>::is_derived_of(base);
	}
	bool IsBaseOf(const HashTypeImpl* derived) const override 
	{
		return derived->IsDerivedOf(this);
	}
	[[nodiscard]] bool IsSerializable() const override 
	{
		return is_serializable_v<T>;
	}
	[[nodiscard]] bool IsInternal() const override
	{
		return is_internal_v<T>;
	}
	[[nodiscard]] std::string_view GetTypeName() const override
	{
		return static_type_name<T>::name();
	}

	constexpr HashTypeT() :
		HashTypeImpl(cityhash::CityHashCrc256_s(static_type_name<T>::full_name().data(), static_type_name<T>::full_name().size())) {}

private:
	friend class boost::serialization::access;

	template <typename Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar& boost::serialization::base_object<HashTypeImpl>(*this);
	}
};

template <typename T>
struct type_hash
{
public:
	static constexpr HashTypeT<T> value{};
};

template <>
struct polymorphic_type_hash<void>
{
	static constexpr size_t upcast_count = 1;
	static constexpr HashArray<upcast_count> upcast_array{ &type_hash<void>::value };

	constexpr static bool is_derived_of(const HashType /*base*/)
	{
		return true;
	}
};

namespace boost::archive
{
	template <typename Archive>
	void load(Archive& ar, const HashTypeImpl*& t)
	{
		// let boost deserialize from file
		const HashTypeImpl* loaded;
		detail::load_pointer_type<Archive>::invoke(ar, loaded);

		// deallocate the memory that was created by boost
		simple_gc_collector::Add(loaded);
		// retrieve the runtime type
		t = loaded->Fetch();
	}
}

template <>
struct std::hash<HashTypeImpl>
{
	constexpr std::size_t operator()(const HashTypeImpl& h) const noexcept
	{
		return h.v.hash();
	}
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
	constexpr static bool is_derived_of(const HashType base)\
	{\
		if constexpr ((upcast_count * sizeof(HashTypeValue)) < (1 << 7))\
		{\
			return std::ranges::find_if(upcast_array, [&base](const auto other){return base->Equal(*other);}) != upcast_array.end();\
		}\
		return std::ranges::binary_search(upcast_array, base, [](const auto lhs, const auto rhs){return *lhs < *rhs;});\
	}\
};

template <typename T, typename = void>
struct is_hash_type : std::false_type {};

template <typename T>
struct is_hash_type<T, std::void_t<decltype(&T::StaticTypeHash)>> : std::true_type {};

template <typename T>
std::atomic<bool>& get_spinlock()
{
    static std::atomic<bool> lock;
    return lock;
}

inline void do_lock( std::atomic<bool>& mtx, const bool value )
{
    bool expected = !value;
    while ( !mtx.compare_exchange_strong( expected, value ) )
    {
    }
}

template <typename T>
void do_lock( const bool value )
{
    std::atomic<bool>& mtx = get_spinlock<T>();
    do_lock( mtx, value );
}

struct AllocationKey
{
    size_t  allocation_count = 0;
    size_t  chunk = 0;
    uint8_t segment = 0;
    uint8_t offset  = 0;

	bool operator==( const AllocationKey& other ) const
	{
        return to_raw_index() == other.to_raw_index() && allocation_count == other.allocation_count;
	}

    [[nodiscard]] size_t to_raw_index() const
	{
        return ( chunk * ( 1 << 8 ) ) + ((size_t)segment * std::numeric_limits<uint32_t>::digits) + (size_t)offset;
	}
};

template <typename T>
class managed_weak_ptr;

template <typename T>
class managed_shared_ptr;

namespace Engine
{
    template <typename T>
    using Weak = managed_weak_ptr<T>;

    template <typename T>
    using Strong = managed_shared_ptr<T>;
} // namespace Engine

struct ENGINE_CORE_API ConstructorAccess
{
    template <typename T>
    friend class object_pool_allocator;

	template <typename T>
    friend class managed_shared_ptr;

    template <typename T, typename... Args>
    inline static Engine::Strong<T> Create( Args&&... args );

private:
    template <typename T, typename... Args>
    inline static void InternalConstruct( T* ptr, Args&&... args )
    {
        ::new ( ptr ) T( std::forward<Args>( args )... );
    }

    template <typename T>
    inline static void InternalDeconstruct( T* ptr )
    {
        ptr->~T();
    }
};

inline static constexpr AllocationKey null_pair = { ( size_t )-1, ( uint8_t )-1, ( uint8_t )-1 };

template <typename T>
class managed_shared_ptr;

struct AllocationContext;

struct deleter_base
{
    virtual ~deleter_base() = default;

    void operator()( void* ptr ) const
    {
        predicate( ptr );
    }

    virtual void predicate( void* shared_ptr ) const = 0;
};

struct null_deleter : deleter_base
{
    void predicate( void* shared_ptr ) const override
    { }
};

class pool_allocator_base
{
public:
    virtual ~pool_allocator_base()
    { }
    virtual void                 destroy()                                 = 0;
    virtual bool                 past( const AllocationContext& ) const    = 0;
    virtual bool                 expired( const AllocationContext& ) const = 0;
    virtual void*                get_ptr( const AllocationContext& ) const = 0;
    virtual size_t               allocation_count() const                  = 0;
    virtual void                 predicate_dealloc( void* ptr ) const      = 0;
};

struct AllocationContext
{
private:
    AllocationKey              m_key_                     = null_pair;
    mutable size_t             m_type_reallocation_count_ = -1;
    const pool_allocator_base* m_allocator_               = nullptr;
    mutable void*              m_ptr_				      = nullptr;

	template <typename T>
    friend class object_pool_allocator;

    template <typename T>
    friend class managed_shared_ptr;

    template <typename T>
    friend class managed_weak_ptr;

	template <typename T>
    friend struct std::hash;

	AllocationContext() = delete;

	struct no_assert_tag
    { };
	
	AllocationContext( no_assert_tag ) {}

public:
	AllocationContext( const AllocationContext& context )
	{
        m_key_                     = context.m_key_;
        m_allocator_               = context.m_allocator_;
        m_type_reallocation_count_ = context.m_type_reallocation_count_;
	}

	static const AllocationContext& get_null_context()
	{
        static AllocationContext null_context = { null_pair, ( size_t )-1, nullptr };
		return null_context;
	}

	bool operator==( const AllocationContext& other ) const noexcept
	{
        return m_key_ == other.m_key_ && m_allocator_ == other.m_allocator_;
	}

	bool valid() const
    {
        return *this != get_null_context();
	}

	bool should_resolve() const
	{
        return m_allocator_->past( *this );
	}

	void update() const
	{
        m_type_reallocation_count_ = m_allocator_->allocation_count();
        m_ptr_                     = m_allocator_->get_ptr( *this );
	}

	bool expired() const
    {
        return m_allocator_->expired( *this );
	}

	void predicate_dealloc( void* ptr ) const
    {
        m_allocator_->predicate_dealloc( ptr );
	}

	void* ptr() const
	{
		if ( should_resolve() )
		{
            update();
		}

		if ( expired() )
        {
            return nullptr;
        }

		if ( !valid() )
        {
            return nullptr;
        }

		return m_ptr_;
	}

	AllocationContext( const AllocationKey& key, size_t type_allocation_count, const pool_allocator_base* allocator )
	{
        assert( key != null_pair && allocator );
        m_key_                     = key;
        m_type_reallocation_count_ = type_allocation_count;
        m_allocator_               = allocator;
        m_ptr_                     = allocator->get_ptr( *this );
	}

	size_t allocation_count() const
	{
        return m_type_reallocation_count_;
	}
};

template <typename T>
class object_pool_allocator;

template <typename T>
class object_pool_allocator : public pool_allocator_base
{
    struct object_tag
    { };
    using PoolType = boost::singleton_pool<object_tag,
                                           sizeof( T ),
                                           boost::default_user_allocator_new_delete,
                                           boost::details::pool::null_mutex>;

    inline static T*                   m_start_ptr_ = nullptr;
    inline static std::vector<__m256i> m_mask_{};
    inline static std::vector<size_t>  m_local_allocation_count_{};
    inline static size_t               m_reallocation_count_ = 0;

    inline static std::atomic<bool> m_lock_        = false;
    inline static bool              m_initialized_ = false;

    static void init()
    {
        if ( !m_initialized_ )
        {
            m_initialized_ = true;
            reallocate();
        }
    }

    static void reallocate()
    {
        T* reallocated = static_cast<T*>( PoolType::ordered_malloc( (m_mask_.size() + 1) * ( 1 << 8 ) ) );
        if ( m_start_ptr_ )
        {
            std::memmove( reallocated, m_start_ptr_, sizeof( T* ) * m_mask_.size() * ( 1 << 8 ) );
            PoolType::ordered_free( m_start_ptr_, m_mask_.size() * ( 1 << 8 ) );
        }
        m_mask_.emplace_back( __m256i{} );
        m_local_allocation_count_.resize( ( m_mask_.size() + 1 ) * ( 1 << 8 ), 0 );
        ++m_reallocation_count_;
    }

    static AllocationKey get_free_space()
    {
        auto it = m_mask_.begin();
        while ( it != m_mask_.end() )
        {
            if ( _mm256_test_all_ones( *it ) )
            {
                ++it;
                continue;
            }

            break;
        }

        const size_t chunk = std::distance( m_mask_.begin(), it );

        if ( it == m_mask_.end() )
        {
            reallocate();
            return { ( size_t )chunk, ( uint8_t )0, ( uint8_t )0 };
        }

        const __mmask8 segment_mask = _mm256_test_epi32_mask( *it, _mm256_cmpeq_epi32( *it, *it ) );
        for ( uint8_t segment = 0; segment < std::numeric_limits<__mmask8>::digits; ++segment )
        {
            if ( !( bool )( ( segment_mask >> ( std::numeric_limits<__mmask8>::digits - 1 ) ) + segment ) )
            {
                const uint8_t offset = ( uint8_t )_tzcnt_u32( ( *it ).m256i_i32[ segment ] );
                return { chunk, segment, offset };
            }
        }

        return { ( size_t )-1, ( uint8_t )-1, ( uint8_t )-1 };
    }

    template <bool Allocate>
    static void flip( T* ptr )
    {
        const size_t dist    = ptr - m_start_ptr_;
        const size_t chunk   = dist % ( 1 << 8 );
        const size_t segment = ( dist % ( ( 1 << 8 ) * chunk ) ) % std::numeric_limits<uint32_t>::digits;
        const size_t offset  = ( dist % ( ( 1 << 8 ) * chunk ) ) % ( segment * std::numeric_limits<uint32_t>::digits );

        if constexpr ( Allocate )
        {
            m_mask_[ chunk ].m256i_i32[ segment ] &= ~( 1 << offset );
        }
        else
        {
            m_mask_[ chunk ].m256i_i32[ segment ] |= 1 << offset;
        }
    }

	static void increase( T* ptr )
	{
        const size_t dist    = ptr - m_start_ptr_;
        ++m_local_allocation_count_[ dist ];
	}

    template <bool Allocate>
    static void flip( const AllocationKey& pair )
    {
        if constexpr ( Allocate )
        {
            m_mask_[ pair.chunk ].m256i_i32[ pair.segment ] &= ~( 1 << pair.offset );
        }
        else
        {
            m_mask_[ pair.chunk ].m256i_i32[ pair.segment ] |= 1 << pair.offset;
        }
    }

	static bool peak(const AllocationKey& pair)
	{
        const uint32_t shift = m_mask_[ pair.chunk ].m256i_i32[ pair.segment ] >> pair.offset;
        const size_t allocation_count = m_local_allocation_count_[ pair.to_raw_index() ];
        return (bool)( shift & 1 ) && pair.allocation_count == allocation_count;
	}
	
	void* start_ptr() const
    {
        return m_start_ptr_;
    }

	static const object_pool_allocator& get_instanced()
	{
        static object_pool_allocator instanced{};
        return instanced;
	}

	static bool is_safe( T* ptr )
	{
        return m_start_ptr_ <= ptr && m_start_ptr_ + ( m_mask_.size() * ( 1 << 8 ) ) > ptr;
	}

public:
    size_t allocation_count() const override
    {
        return m_reallocation_count_;
    }

    template <typename... Args>
    static AllocationContext allocate( Args&&... arguments )
    {
        init();
        bool entry_expected  = false;
        bool return_expected = true;

        while ( !m_lock_.compare_exchange_strong( entry_expected, true ) )
        {
        }
        AllocationKey             next_pair = get_free_space();
        const object_pool_allocator& instanced = get_instanced();
        AllocationContext context( next_pair, m_reallocation_count_, &instanced );
        flip<true>( next_pair );
        while ( !m_lock_.compare_exchange_strong( return_expected, false ) )
        {
        }
        ConstructorAccess::InternalConstruct<T>( m_start_ptr_ + next_pair.to_raw_index(),
                                                 std::forward<Args>( arguments )... );

        return context;
    }

    static void deallocate( T* ptr )
    {
        assert( is_safe( ptr ) );
        bool entry_expected  = false;
        bool return_expected = true;

		ConstructorAccess::InternalDeconstruct( ptr );
        while ( !m_lock_.compare_exchange_strong( entry_expected, true ) )
        {
        }
        increase( ptr );
        flip<false>( ptr );
        while ( !m_lock_.compare_exchange_strong( return_expected, false ) )
        {
        }
    }

	void predicate_dealloc( void* instance ) const override
	{
        boost::shared_ptr<T>* shared = static_cast<boost::shared_ptr<T>*>( instance );
        if ( shared && *shared && shared->use_count() == 1 )
		{
            deallocate( shared->get() );
		}
	}

	bool past( const AllocationContext& context ) const override
	{
        return context.allocation_count() != m_reallocation_count_;
	}

	bool expired( const AllocationContext& context ) const override
	{
		return peak( context.m_key_ );
	}

	void* get_ptr( const AllocationContext& context ) const override
	{
		if ( !context.valid() )
		{
            return nullptr;
		}

		if ( context.expired() )
		{
            return nullptr;
		}

        T* ptr = m_start_ptr_ + context.m_key_.to_raw_index();
		if ( is_safe(ptr) )
		{
            return ptr;
		}

        return nullptr;
	}

	static AllocationContext get_context( T* ptr )
	{
        const size_t dist    = ptr - m_start_ptr_;
        const size_t chunk   = dist % ( 1 << 8 );
        const size_t segment = ( dist % ( ( 1 << 8 ) * chunk ) ) % std::numeric_limits<uint32_t>::digits;
        const size_t offset  = ( dist % ( ( 1 << 8 ) * chunk ) ) % ( segment * std::numeric_limits<uint32_t>::digits );
        const size_t allocation_count = m_local_allocation_count_[ dist ];
        const object_pool_allocator& instanced        = get_instanced();

        return AllocationContext(
                { allocation_count, chunk, ( uint8_t )segment, ( uint8_t )offset }, m_reallocation_count_, &instanced );
	}

    virtual void destroy() override
    {
        PoolType::ordered_free( m_start_ptr_, m_mask_.size() * ( 1 << 8 ) );
        PoolType::release_memory();
        PoolType::purge_memory();
    }
};

class PoolAllocatorStorage
{
    std::unordered_set<std::unique_ptr<pool_allocator_base>> m_allocators_ = {};

public:
	template <typename T>
    void register_allocator()
    {
        m_allocators_.emplace( std::make_unique<object_pool_allocator<T>>() );
    }

	void cleanup()
	{
	    for ( const std::unique_ptr<pool_allocator_base>& allocator : m_allocators_ )
	    {
            allocator->destroy();
	    }
	}
};

inline static PoolAllocatorStorage g_allocator_storage{};

namespace Engine
{
    struct Serializer;
}

template <typename T>
class managed_shared_ptr : protected boost::shared_ptr<T>
{
    mutable AllocationContext m_context_;

	template <typename U> 
	friend class managed_weak_ptr;
    
	template <typename U>
    friend class managed_shared_ptr;

    friend struct std::hash<managed_shared_ptr>;

	friend struct Engine::Serializer;

	friend class boost::serialization::access;
	
    template <typename T1, typename T2>
    friend managed_shared_ptr<T1> managed_reinterpret_pointer_cast( const managed_shared_ptr<T2>& r ) noexcept;

	template <typename T, typename... Args>
    friend managed_shared_ptr<T> make_managed_shared( Args&&... arguments );

    template <typename T, typename U, typename... Args>
        requires std::is_convertible_v<U*, T*>
    friend managed_shared_ptr<U> make_managed_shared( Args&&... arguments );

	template <typename To, typename From>
    friend managed_shared_ptr<To> from_native_shared( const boost::shared_ptr<From>& native );

	struct reinterpret_tag
    { };

	template <typename U>
    managed_shared_ptr( const managed_shared_ptr<U>& other, const reinterpret_tag& ) : m_context_( other.m_context_ )
	{
        resolve();
	}

	managed_shared_ptr( const AllocationContext& context ) : m_context_( context )
	{
        resolve();
	}

	AllocationContext internal_context() const
    {
        return m_context_;
    }

    [[nodiscard]] boost::shared_ptr<T> native() const
    {
        resolve();
        return *this;
    }

	template <class Archive>
    void serialize( Archive& ar, const unsigned int file_version )
    {
        ar & static_cast<boost::shared_ptr<T>&>( *this );
	}

public:
	~managed_shared_ptr()
	{
        resolve();
        m_context_.predicate_dealloc( this );
	}

	managed_shared_ptr( nullptr_t )
        : boost::shared_ptr<T>( nullptr, null_deleter{} ), m_context_( AllocationContext::get_null_context()  )
    {
	}

	managed_shared_ptr()
        : boost::shared_ptr<T>( nullptr, null_deleter{} ), m_context_( AllocationContext::get_null_context()  )
	{
	}

	template <typename U> requires std::is_convertible_v<U*, T*>
    managed_shared_ptr( const managed_shared_ptr<U>& other ) : m_context_( other.m_context_ ), boost::shared_ptr<T>( other )
    {
        resolve();
    }

    managed_shared_ptr( const managed_shared_ptr& other ) : m_context_( other.m_context_ )
	{
        m_context_ = other.m_context_;
        boost::shared_ptr<T>::reset( other.get(), null_deleter{} );
	}

	bool operator==( nullptr_t )
	{
        return !m_context_.valid();
	}

    operator bool() const noexcept
    {
        return !m_context_.valid();
	}

	operator boost::shared_ptr<T>() const
	{
        resolve();
        boost::shared_ptr<T> result( *this );
        return result;
	}

	void reset()
	{
        m_context_ = AllocationContext::get_null_context();
        boost::shared_ptr<T>::reset();
	}

	T* resolve() const noexcept
    {
		if ( !m_context_.valid() )
		{
			return nullptr;
		}

		if ( m_context_.expired() )
		{
			if ( boost::shared_ptr<T>::get() )
			{
                managed_shared_ptr<T>* non_const = const_cast<managed_shared_ptr<T>*>( this );
                static_cast<boost::shared_ptr<T>*>( non_const )->reset();
                m_context_ = AllocationContext::get_null_context();
			}
            return nullptr;
		}

		if ( m_context_.should_resolve() )
		{
            m_context_.update();
			managed_shared_ptr<T>* non_const = const_cast<managed_shared_ptr<T>*>( this );
            static_cast<boost::shared_ptr<T>*>( non_const )->reset( static_cast<T*>( m_context_.ptr() ), null_deleter{} );
        }

        return boost::shared_ptr<T>::get();
	}

	[[nodiscard]] size_t use_count() const
	{
        return boost::shared_ptr<T>::use_count();
	}

	bool operator!() const noexcept
    {
        return resolve() == nullptr;
	}

	T& operator*() const noexcept
	{
        return *resolve();
	}

	T* operator->() const noexcept
	{
        return resolve();
	}

	T* get() const noexcept
	{
        return resolve();
	}

    [[nodiscard]] managed_weak_ptr<T> to_weak() const
	{
        return managed_weak_ptr( *this );
	}
};

template <class T, class U> requires std::is_convertible_v<T*, U*>
inline bool operator==( managed_shared_ptr<T> const& a, managed_shared_ptr<U> const& b ) noexcept
{
    return a.get() == b.get();
}

template <class T>
inline bool operator==( managed_shared_ptr<T> const& a, nullptr_t ) noexcept
{
    return a.get() == nullptr;
}

template <class T>
inline bool operator!=( managed_shared_ptr<T> const& a, nullptr_t ) noexcept
{
    return a.get() != nullptr;
}

template <class T, class U>
managed_shared_ptr<T> managed_reinterpret_pointer_cast( const managed_shared_ptr<U>& r ) noexcept
{
    static typename managed_shared_ptr<T>::reinterpret_tag placeholder{};
    return managed_shared_ptr<T>( r, placeholder );
}

template <typename T, typename U> requires std::is_convertible_v<U*, T*> || std::is_convertible_v<T*, U*>
managed_shared_ptr<T> managed_static_pointer_cast( const managed_shared_ptr<U>& r ) noexcept
{
    return managed_reinterpret_pointer_cast<T>( r );
}

template <typename T>
class managed_weak_ptr : protected boost::weak_ptr<T>
{
    mutable AllocationContext m_context_;

	template <typename U>
    friend class managed_weak_ptr;

public:
    managed_weak_ptr() : boost::weak_ptr<T>( {} ), m_context_( AllocationContext::no_assert_tag{} )
    {
        m_context_ = AllocationContext::get_null_context();
    }

	template <typename U> requires std::is_convertible_v<U*, T*>
	managed_weak_ptr( const managed_weak_ptr<U>& other )
		: boost::weak_ptr<T>( other ),
		m_context_( other.m_context_ )
	{ }

    managed_weak_ptr( const managed_shared_ptr<T>& other )
        : boost::weak_ptr<T>( other ), m_context_( other.m_context_ )
    { }

	template <typename U> requires std::is_convertible_v<U*, T*>
    managed_weak_ptr( const managed_shared_ptr<U>& other ) : boost::weak_ptr<T>( other ), m_context_( other.m_context_ )
    { }

	void reset()
    {
        m_context_ = AllocationContext::get_null_context();
		boost::weak_ptr<T>::reset();
    }

	bool operator==(const managed_weak_ptr<T>& other) const
	{
        return m_context_ == other.m_context_;
	}

	[[nodiscard]] bool expired() const
    {
        return m_context_.expired();
	}

	[[nodiscard]] bool empty() const
    {
        return m_context_ == AllocationContext::get_null_context();
	}

	managed_shared_ptr<T> lock() const
    {
        if ( !boost::weak_ptr<T>::lock() )
        {
            m_context_ = AllocationContext::get_null_context();
        }

		return managed_shared_ptr<T>( m_context_ );
    }
};

template <class T>
inline bool operator==( managed_weak_ptr<T> const& a, nullptr_t ) noexcept
{
    return a.lock() == nullptr;
}

template <class T>
inline bool operator!=( managed_weak_ptr<T> const& a, nullptr_t ) noexcept
{
    return a.lock() != nullptr;
}

template <typename T>
struct std::hash<managed_shared_ptr<T>>
{
    size_t operator()( const managed_shared_ptr<T>& p ) const noexcept
    {
		if ( p.m_context_.expired() )
		{
            return 0;
		}

        static std::hash<size_t> hasher{};
        size_t                   seed = hasher( p.m_context_.m_key_.to_raw_index() );
		boost::hash_combine( seed, hasher( reinterpret_cast<size_t>( p.get() ) ) );
        return seed;
    }
};

template <typename T>
struct std::hash<managed_weak_ptr<T>>
{
    size_t operator()( const managed_weak_ptr<T>& p ) const noexcept
    {
        if ( p.expired() )
        {
            return 0;
        }

		static std::hash<managed_shared_ptr<T>> hasher{};
        return hasher( p.lock() );
    }
};

template <typename T, typename U>
inline bool operator<( const managed_shared_ptr<T>& left, const managed_shared_ptr<U>& right ) noexcept
{
    return reinterpret_cast<uintptr_t>( left.get() ) < reinterpret_cast<uintptr_t>( right.get() );
}

template <typename T, typename U>
inline bool operator<( const managed_weak_ptr<T>& left, const managed_weak_ptr<U>& right ) noexcept
{
    return left.lock() < right.lock();
}

template <typename T, typename... Args>
managed_shared_ptr<T> make_managed_shared( Args&&... arguments )
{
    g_allocator_storage.register_allocator<T>();
    return object_pool_allocator<T>::allocate( std::forward<Args>( arguments )... );
}

template <typename T, typename U, typename... Args>
    requires std::is_convertible_v<U*, T*>
managed_shared_ptr<U> make_managed_shared( Args&&... arguments )
{
    g_allocator_storage.register_allocator<T>();
    return object_pool_allocator<T>::allocate( std::forward<Args>( arguments )... );
}

template <typename To, typename From>
managed_shared_ptr<To> from_native_shared( const boost::shared_ptr<From>& native )
{
	if ( !native )
	{
        return {};
	}

	AllocationContext key = object_pool_allocator<To>::get_context( static_cast<To*>( native.get() ) );
    return managed_shared_ptr<To>( key );
}

template <typename ValueType, typename... Args> requires is_hash_type<ValueType>::value
struct FactoryTemplate
{
public:
	using GeneratorSignature = std::function<Engine::Strong<ValueType>(Args...)>;
	using GeneratorContainer = std::unordered_map<HashType, GeneratorSignature>;

	static GeneratorSignature GetGenerator(HashType key)
	{
		if (m_generators_.contains(key))
		{
			return m_generators_.at(key);
		}

		return {};
	}

	static const GeneratorContainer& GetGenerators()
	{
		return m_generators_;
	}

	template <typename T> requires std::is_base_of_v<ValueType, T>
	static void Register()
	{
		m_generators_.emplace(T::StaticTypeHash(), &FactoryTemplate::Create<T>);
	}

	template <typename T> requires std::is_base_of_v<ValueType, T>
	static void Unregister()
	{
		if (m_generators_.contains(T::StaticTypeHash()))
		{
			m_generators_.erase(T::StaticTypeHash());
		}
	}

	template <typename T> requires std::is_base_of_v<ValueType, T>
	static Engine::Strong<ValueType> Create(Args&&... args)
	{
		return ConstructorAccess::Create<T>(std::forward<Args>(args)...);
	}

private:
	inline static GeneratorContainer m_generators_ = {};
};

template <typename T, typename... Args>
inline Engine::Strong<T> ConstructorAccess::Create( Args&&... args )
{
    return make_managed_shared<T>( std::forward<Args>( args )... );
}

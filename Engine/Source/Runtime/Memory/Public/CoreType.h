#pragma once

#include "Macro.h"

#include <algorithm>
#include <array>
#include <vector>
#include <cstddef>
#include <magic_enum/magic_enum.hpp>
#include <string_view>
#include <stdint.h>
#include <stdexcept>
#include <unordered_set>
#include <map>
#include <memory>
#include <mutex>
#include <ranges>
#include <type_traits>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/archive/detail/iserializer.hpp>
#include <boost/functional/hash.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/pool/pool_alloc.hpp>

namespace Engine
{
    struct ModuleInfo;
}

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

struct ENGINE_MEMORY_API simple_gc_deleter_impl
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

class ENGINE_MEMORY_API byte_stream
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

struct ENGINE_MEMORY_API simple_gc_collector
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

struct ENGINE_MEMORY_API simple_gc_scope
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
    static constexpr const uint32_t table[ 256 ] = 
	{
        0x00000000U, 0x77073096U, 0xEE0E612CU, 0x990951BAU, 0x076DC419U, 0x706AF48FU, 0xE963A535U, 0x9E6495A3U,
        0x0EDB8832U, 0x79DCB8A4U, 0xE0D5E91EU, 0x97D2D988U, 0x09B64C2BU, 0x7EB17CBDU, 0xE7B82D07U, 0x90BF1D91U,
        0x1DB71064U, 0x6AB020F2U, 0xF3B97148U, 0x84BE41DEU, 0x1ADAD47DU, 0x6DDDE4EBU, 0xF4D4B551U, 0x83D385C7U,
        0x136C9856U, 0x646BA8C0U, 0xFD62F97AU, 0x8A65C9ECU, 0x14015C4FU, 0x63066CD9U, 0xFA0F3D63U, 0x8D080DF5U,
        0x3B6E20C8U, 0x4C69105EU, 0xD56041E4U, 0xA2677172U, 0x3C03E4D1U, 0x4B04D447U, 0xD20D85FDU, 0xA50AB56BU,
        0x35B5A8FAU, 0x42B2986CU, 0xDBBBC9D6U, 0xACBCF940U, 0x32D86CE3U, 0x45DF5C75U, 0xDCD60DCFU, 0xABD13D59U,
        0x26D930ACU, 0x51DE003AU, 0xC8D75180U, 0xBFD06116U, 0x21B4F4B5U, 0x56B3C423U, 0xCFBA9599U, 0xB8BDA50FU,
        0x2802B89EU, 0x5F058808U, 0xC60CD9B2U, 0xB10BE924U, 0x2F6F7C87U, 0x58684C11U, 0xC1611DABU, 0xB6662D3DU,
        0x76DC4190U, 0x01DB7106U, 0x98D220BCU, 0xEFD5102AU, 0x71B18589U, 0x06B6B51FU, 0x9FBFE4A5U, 0xE8B8D433U,
        0x7807C9A2U, 0x0F00F934U, 0x9609A88EU, 0xE10E9818U, 0x7F6A0DBBU, 0x086D3D2DU, 0x91646C97U, 0xE6635C01U,
        0x6B6B51F4U, 0x1C6C6162U, 0x856530D8U, 0xF262004EU, 0x6C0695EDU, 0x1B01A57BU, 0x8208F4C1U, 0xF50FC457U,
        0x65B0D9C6U, 0x12B7E950U, 0x8BBEB8EAU, 0xFCB9887CU, 0x62DD1DDFU, 0x15DA2D49U, 0x8CD37CF3U, 0xFBD44C65U,
        0x4DB26158U, 0x3AB551CEU, 0xA3BC0074U, 0xD4BB30E2U, 0x4ADFA541U, 0x3DD895D7U, 0xA4D1C46DU, 0xD3D6F4FBU,
        0x4369E96AU, 0x346ED9FCU, 0xAD678846U, 0xDA60B8D0U, 0x44042D73U, 0x33031DE5U, 0xAA0A4C5FU, 0xDD0D7CC9U,
        0x5005713CU, 0x270241AAU, 0xBE0B1010U, 0xC90C2086U, 0x5768B525U, 0x206F85B3U, 0xB966D409U, 0xCE61E49FU,
        0x5EDEF90EU, 0x29D9C998U, 0xB0D09822U, 0xC7D7A8B4U, 0x59B33D17U, 0x2EB40D81U, 0xB7BD5C3BU, 0xC0BA6CADU,
        0xEDB88320U, 0x9ABFB3B6U, 0x03B6E20CU, 0x74B1D29AU, 0xEAD54739U, 0x9DD277AFU, 0x04DB2615U, 0x73DC1683U,
        0xE3630B12U, 0x94643B84U, 0x0D6D6A3EU, 0x7A6A5AA8U, 0xE40ECF0BU, 0x9309FF9DU, 0x0A00AE27U, 0x7D079EB1U,
        0xF00F9344U, 0x8708A3D2U, 0x1E01F268U, 0x6906C2FEU, 0xF762575DU, 0x806567CBU, 0x196C3671U, 0x6E6B06E7U,
        0xFED41B76U, 0x89D32BE0U, 0x10DA7A5AU, 0x67DD4ACCU, 0xF9B9DF6FU, 0x8EBEEFF9U, 0x17B7BE43U, 0x60B08ED5U,
        0xD6D6A3E8U, 0xA1D1937EU, 0x38D8C2C4U, 0x4FDFF252U, 0xD1BB67F1U, 0xA6BC5767U, 0x3FB506DDU, 0x48B2364BU,
        0xD80D2BDAU, 0xAF0A1B4CU, 0x36034AF6U, 0x41047A60U, 0xDF60EFC3U, 0xA867DF55U, 0x316E8EEFU, 0x4669BE79U,
        0xCB61B38CU, 0xBC66831AU, 0x256FD2A0U, 0x5268E236U, 0xCC0C7795U, 0xBB0B4703U, 0x220216B9U, 0x5505262FU,
        0xC5BA3BBEU, 0xB2BD0B28U, 0x2BB45A92U, 0x5CB36A04U, 0xC2D7FFA7U, 0xB5D0CF31U, 0x2CD99E8BU, 0x5BDEAE1DU,
        0x9B64C2B0U, 0xEC63F226U, 0x756AA39CU, 0x026D930AU, 0x9C0906A9U, 0xEB0E363FU, 0x72076785U, 0x05005713U,
        0x95BF4A82U, 0xE2B87A14U, 0x7BB12BAEU, 0x0CB61B38U, 0x92D28E9BU, 0xE5D5BE0DU, 0x7CDCEFB7U, 0x0BDBDF21U,
        0x86D3D2D4U, 0xF1D4E242U, 0x68DDB3F8U, 0x1FDA836EU, 0x81BE16CDU, 0xF6B9265BU, 0x6FB077E1U, 0x18B74777U,
        0x88085AE6U, 0xFF0F6A70U, 0x66063BCAU, 0x11010B5CU, 0x8F659EFFU, 0xF862AE69U, 0x616BFFD3U, 0x166CCF45U,
        0xA00AE278U, 0xD70DD2EEU, 0x4E048354U, 0x3903B3C2U, 0xA7672661U, 0xD06016F7U, 0x4969474DU, 0x3E6E77DBU,
        0xAED16A4AU, 0xD9D65ADCU, 0x40DF0B66U, 0x37D83BF0U, 0xA9BCAE53U, 0xDEBB9EC5U, 0x47B2CF7FU, 0x30B5FFE9U,
        0xBDBDF21CU, 0xCABAC28AU, 0x53B39330U, 0x24B4A3A6U, 0xBAD03605U, 0xCDD70693U, 0x54DE5729U, 0x23D967BFU,
        0xB3667A2EU, 0xC4614AB8U, 0x5D681B02U, 0x2A6F2B94U, 0xB40BBE37U, 0xC30C8EA1U, 0x5A05DF1BU, 0x2D02EF8DU
    };

	constexpr uint32_t compute( const char* data, uint32_t len, uint32_t crc = 0 )
    {
        crc = crc ^ 0xFFFFFFFFU;
        for ( uint32_t i = 0; i < len; i++ )
        {
            crc = table[ *data ^ ( crc & 0xFF ) ] ^ ( crc >> 8 );
            data++;
        }
        crc = crc ^ 0xFFFFFFFFU;
        return crc;
    }

	constexpr uint32_t compute( uint64_t data, uint32_t crc = 0 )
    {
        crc = crc ^ 0xFFFFFFFFU;
        for ( uint32_t i = 0; i < std::numeric_limits<uint64_t>::digits; ++i )
        {
            crc = table[ static_cast<char>( data & 0x1 ) ^ ( crc & 0xFF ) ] ^ ( crc >> 8 );
            data >>= 1;
        }
        crc = crc ^ 0xFFFFFFFFU;
        return crc;
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
    struct ENGINE_MEMORY_API cityhash256
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
    z = crc32::compute( b + g, z );     \
    y = crc32::compute( e + h, y );     \
    x = crc32::compute( f + a, x );     \
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

	constexpr cityhash256 CityHashCrc256_s(const char* s, const size_t len) 
	{
		cityhash256 retval{};
		
		if ( LIKELY( len  >= 240 ) )
        {
            CityHashCrc256Long( s, len, 0, retval.v );
		}
		else {
            CityHashCrc256Short( s, len, retval.v );
		}

		return retval;
	}
#undef CHUNK
}

BOOST_CLASS_EXPORT_KEY(cityhash::cityhash256)

struct ENGINE_MEMORY_API typename_prober
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
	constexpr static bool StaticIsDerivedOf(HashType base) \
	{ \
		return polymorphic_type_hash<##Type##>::is_derived_of(base); \
	} \
	virtual std::string_view GetTypeName() const { return Type##::StaticFullTypeName(); } \
	virtual std::string_view GetPrettyTypeName() const { return Type##::StaticTypeName(); } \
	virtual HashType GetTypeHash() const { return Type##::StaticTypeHash(); } \
	virtual bool IsDerivedOf(HashType base) const { return Type##::StaticIsDerivedOf(base); } \
	virtual bool IsBaseOf(HashType derived) const { return derived->IsDerivedOf(Type##::StaticTypeHash()); }

class HashType;

struct ENGINE_MEMORY_API HashTypeImpl
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
	[[nodiscard]] virtual HashType Fetch() const;
	[[nodiscard]] virtual bool     IsDerivedOf(HashType base) const;
	[[nodiscard]] virtual bool     IsBaseOf(HashType derived) const;
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

	constexpr HashTypeImpl() : v() {}
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

// Type identity handle (points at a type_hash<T>::value).
// type_hash<T>::value is instantiated once per DLL, so the same type can have different addresses; its cityhash `v`
// is evaluated at compile time and identical everywhere. Equality is therefore decided in three steps:
//   1) both operands are constant expressions -> the compiler compares the AOT hashes (no runtime cost);
//   2) same address -> equal, without reading the hash (the common same-DLL case);
//   3) otherwise -> full comparison of the AOT hashes.
// A different address alone never means a different type. Use SameAddress()/get() only when the address itself is
// what you want (convenience within one module); it can report "different" for one type across DLLs.
class HashType
{
public:
	constexpr HashType() noexcept = default;
	constexpr HashType(std::nullptr_t) noexcept {}
	constexpr HashType(const HashTypeImpl* ptr) noexcept : m_ptr_(ptr) {}

	[[nodiscard]] constexpr const HashTypeImpl* get() const noexcept { return m_ptr_; }
	constexpr const HashTypeImpl*               operator->() const noexcept { return m_ptr_; }
	constexpr const HashTypeImpl&               operator*() const noexcept { return *m_ptr_; }
	constexpr explicit                          operator bool() const noexcept { return m_ptr_ != nullptr; }

	[[nodiscard]] constexpr bool SameAddress(const HashType other) const noexcept { return m_ptr_ == other.m_ptr_; }

	[[nodiscard]] constexpr bool Equals(const HashType other) const noexcept
	{
		return m_ptr_ == other.m_ptr_ || ( m_ptr_ && other.m_ptr_ && m_ptr_->v == other.m_ptr_->v );
	}

	[[nodiscard]] constexpr size_t hash() const noexcept { return m_ptr_ ? m_ptr_->v.hash() : 0; }

	friend constexpr bool operator==(const HashType lhs, const HashType rhs) noexcept { return lhs.Equals(rhs); }
	friend constexpr bool operator==(const HashType lhs, std::nullptr_t) noexcept { return lhs.m_ptr_ == nullptr; }

	// Strict weak order by hash value (null first), consistent with ==.
	friend constexpr bool operator<(const HashType lhs, const HashType rhs) noexcept
	{
		if ( !lhs.m_ptr_ || !rhs.m_ptr_ )
		{
			return !lhs.m_ptr_ && rhs.m_ptr_;
		}
		return !lhs.SameAddress(rhs) && lhs.m_ptr_->v < rhs.m_ptr_->v;
	}

private:
	const HashTypeImpl* m_ptr_ = nullptr;
};

template <>
struct std::hash<HashType>
{
	constexpr size_t operator()(const HashType h) const noexcept { return h.hash(); }
};

struct HashTypeHash
{
	constexpr size_t operator()( const HashType p ) const noexcept { return p.hash(); }
};

// Serialized exactly like the raw `const HashTypeImpl*` it replaced: no class header, no tracking of the handle,
// the pointee goes through the existing pointer path (and boost::archive::load -> Fetch() on load).
namespace boost::serialization
{
	template <typename Archive>
	void save(Archive& ar, const HashType& t, const unsigned int /*version*/)
	{
		const HashTypeImpl* ptr = t.get();
		ar & ptr;
	}

	template <typename Archive>
	void load(Archive& ar, HashType& t, const unsigned int /*version*/)
	{
		const HashTypeImpl* ptr = nullptr;
		ar & ptr;
		t = ptr;
	}
}

BOOST_SERIALIZATION_SPLIT_FREE(HashType)
BOOST_CLASS_IMPLEMENTATION(HashType, boost::serialization::object_serializable)
BOOST_CLASS_TRACKING(HashType, boost::serialization::track_never)

inline HashType HashTypeImpl::Fetch() const
{
	throw std::runtime_error("Not Implemented");
}

inline bool HashTypeImpl::IsDerivedOf(HashType /*base*/) const
{
	throw std::runtime_error("Not Implemented");
}

inline bool HashTypeImpl::IsBaseOf(HashType /*derived*/) const
{
	throw std::runtime_error("Not Implemented");
}

template <typename T> struct type_hash;

template <size_t Count>
using HashArray = std::array<HashType, Count>;

// Compile-time inheritance chain typelist for polymorphic_type_hash.
// chain = type_list<MostDerived, DirectBase, ..., void> (one path; POLYMORPHIC_TYPE_MAP registers one immediate base per type).
// Virtual inheritance: the chain still lists each type once along that path (e.g. IGraphicAPI -> IGraphicAPIBase).
// Types with multiple direct bases have one chain per POLYMORPHIC_TYPE_MAP; pointer conversion from virtual base to derived
// cannot use static_cast and must use dynamic_cast when RTTI is available (see safe_cast).
template <typename... Ts>
struct type_list {};

template <typename T, typename List>
struct type_list_prepend;

template <typename T, typename... Ts>
struct type_list_prepend<T, type_list<Ts...>>
{
	using type = type_list<T, Ts...>;
};

// True if U appears in the chain (direct or indirect base).
template <typename U, typename List>
struct type_list_contains : std::false_type {};

template <typename U>
struct type_list_contains<U, type_list<>> : std::false_type {};

template <typename U, typename First, typename... Rest>
struct type_list_contains<U, type_list<First, Rest...>>
	: std::conditional_t<std::is_same_v<U, First>, std::true_type, type_list_contains<U, type_list<Rest...>>> {};

template <typename List>
struct type_list_size;

template <typename... Ts>
struct type_list_size<type_list<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

template <typename T>
struct polymorphic_type_hash
{
	static constexpr size_t upcast_count = 0;
	static constexpr HashArray<upcast_count> upcast_array{};
	using chain = type_list<T>;

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
		return this == &other || HashTypeImpl::Equal(other);
	}
	[[nodiscard]] HashType Fetch() const override
	{
		return &type_hash<T>::value;
	}
	bool IsDerivedOf(HashType base) const override
	{
		return polymorphic_type_hash<T>::is_derived_of(base);
	}
	bool IsBaseOf(HashType derived) const override 
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

// Derive runtime upcast array from the chain (single source of truth).
template <typename List>
struct chain_to_upcast_array;

template <typename... Ts>
struct chain_to_upcast_array<type_list<Ts...>>
{
	static constexpr size_t count = sizeof...(Ts);
	static constexpr HashArray<count> value = { &type_hash<Ts>::value... };
};

template <>
struct polymorphic_type_hash<void>
{
	using chain = type_list<void>;
	static constexpr size_t upcast_count = type_list_size<chain>::value;
	static constexpr HashArray<upcast_count> upcast_array = chain_to_upcast_array<chain>::value;

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
		t = loaded->Fetch().get();
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
	using chain = type_list_prepend<Type, polymorphic_type_hash<##Base##>::chain>::type;\
	static constexpr size_t upcast_count = type_list_size<chain>::value;\
	static constexpr HashArray<upcast_count> upcast_array = chain_to_upcast_array<chain>::value;\
	constexpr static bool is_derived_of(const HashType base)\
	{\
		return base && std::ranges::find(upcast_array, base) != upcast_array.end();\
	}\
};

template <typename T, typename = void>
struct is_hash_type : std::false_type {};

template <typename T>
struct is_hash_type<T, std::void_t<decltype(&T::StaticTypeHash)>> : std::true_type {};

// RTTI-free replacement for dynamic_cast<T*>(ptr). Uses HashType + IsDerivedOf (no vtable comparison).
// Requires: U has GetTypeHash() (e.g. INLINE_COMPILE_TIME_TYPENAME) and POLYMORPHIC_TYPE_MAP(U, ...) for the hierarchy.
template <typename T, typename U>
T* safe_cast( U* ptr ) noexcept
{
	if ( !ptr )
		return nullptr;
	if constexpr ( is_hash_type<std::remove_cv_t<U>>::value )
	{
		if ( ptr->GetTypeHash()->IsDerivedOf( &type_hash<T>::value ) )
			return static_cast<T*>( ptr );
	}
	return nullptr;
}

// RTTI-free replacement for dynamic_cast<T&>(*ptr). Throws std::bad_cast if the cast fails.
template <typename T, typename U>
T& safe_cast_ref( U* ptr )
{
	T* p = safe_cast<T>( ptr );
	if ( !p )
		throw std::bad_cast();
	return *p;
}

template <typename T, typename = void>
struct is_predtor_defined : std::false_type
{ };

template <typename T>
struct is_predtor_defined<T, std::void_t<decltype( &T::PreDeconstruction )>> : std::true_type
{ };


template <typename T>
std::atomic<bool>& get_spinlock()
{
    static std::atomic<bool> lock;
    return lock;
}

inline void do_lock( std::atomic<bool>& mtx, const bool value )
{
    if ( value )
    {
        while ( mtx.exchange( true ) )
        { }
    }
    else
    {
        mtx.store( false );
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
    size_t  allocation_count = ( size_t )-1;
    size_t  chunk            = ( size_t )-1;
    uint8_t segment          = ( uint8_t )-1;
    uint8_t offset           = ( uint8_t )-1;

	bool operator==( const AllocationKey& other ) const
	{
        return to_raw_index() == other.to_raw_index() && allocation_count == other.allocation_count;
	}

	bool valid() const
	{
        return allocation_count == ( size_t )-1 && chunk == ( size_t )-1 && segment == ( uint8_t )-1 &&
               offset == ( uint8_t )-1;
	}

    [[nodiscard]] size_t to_raw_index() const
	{
		if (allocation_count == (size_t)-1 || chunk == (size_t)-1 || segment == (uint8_t)-1 || offset == (uint8_t)-1)
		{
			// todo: invalid index
            return (size_t)-1;
		}

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

struct ENGINE_MEMORY_API ConstructorAccess
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
	inline static void InternalPreDeconstruction( T* ptr )
	{
		if constexpr (is_predtor_defined<T>::value == true)
		{
            ptr->PreDeconstruction();
		}
	}

    template <typename T>
    inline static void InternalDeconstruct( T* ptr )
    {
        ptr->~T();
    }
};

inline static constexpr AllocationKey null_pair = { (size_t)-1, ( size_t )-1, ( uint8_t )-1, ( uint8_t )-1 };

template <typename T>
class managed_shared_ptr;

struct AllocationContext;

struct ENGINE_MEMORY_API deleter_base
{
    virtual ~deleter_base() = default;

    void operator()( void* ptr ) const
    {
        predicate( ptr );
    }

    virtual void predicate( void* shared_ptr ) const = 0;
};

struct ENGINE_MEMORY_API null_deleter : deleter_base
{
    void predicate( void* shared_ptr ) const override
    { }
};

// Type-independent slot bookkeeping of one object pool (slot mask, per-slot generation, storage, address
// arithmetic). Created and owned by PoolAllocatorStorage in Memory.dll and never destroyed, and its member
// functions are compiled into Memory.dll: an AllocationContext held by any module stays usable after the module
// that first used the type is unloaded. Only construction and destruction of T live in the typed
// object_pool_allocator<T>.
//
// Storage never moves: the objects, the slot mask and the generations each live in an address range reserved up
// front and committed 256 slots at a time as the pool grows (reserve_bytes is the object range; the generations
// take another 8 bytes per slot and the mask 32 bytes per 256 slots). Addresses of objects and of the bookkeeping
// stay valid until the process exits, so readers that skip the lock never touch freed memory (the values they read
// can still race with a writer holding the lock).
class ENGINE_MEMORY_API pool_core
{
    // Slots are reserved, freed and addressed only by the typed front and the managed side: nothing else can free
    // an object under a managed pointer or reuse its slot.
    template <typename T>
    friend class object_pool_allocator;
    friend struct managed_pool_access;

public:
    pool_core( size_t element_size, size_t reserve_bytes );
    pool_core( const pool_core& )            = delete;
    pool_core& operator=( const pool_core& ) = delete;

    size_t element_size() const { return m_element_size_; }
    size_t reserve_bytes() const { return m_reserve_bytes_; }
    size_t allocation_count() const { return m_reallocation_count_; }

    // How the objects of a pool are owned. A pool is either managed (objects owned by managed pointers, freed only
    // by their control block) or unmanaged (allocate_unmanaged/deallocate_unmanaged); the first allocation decides.
    enum class ownership : uint8_t { undecided, managed, unmanaged };
    ownership current_ownership() const { return m_ownership_.load( std::memory_order_relaxed ); }

    // True while the slot of `key` still holds that allocation (its generation did not move). Lock-free: the
    // generations never move and a slot's generation only changes after its object was destroyed, which cannot
    // happen while a managed pointer owns it; a concurrent write to the same slot only happens in that bug case.
    bool is_current( const AllocationKey& key ) const
    {
        return m_local_allocation_count_[ key.to_raw_index() ] == key.allocation_count;
    }

    // Ends the process (__fastfail) instead of continuing with an object that is gone or not owned this way.
    [[noreturn]] static void fail_fast( const char* reason );

    bool  past( const AllocationContext& context ) const;
    bool  expired( const AllocationContext& context ) const;
    void* get_ptr( const AllocationContext& context ) const;

    void destroy() const;
    void purge() const;

private:
    // Records the pool's ownership on first use; ends the process if the pool is already used the other way.
    void claim_ownership( ownership mode );
    // Takes a free slot under the lock (growing the storage if needed) and returns its context, also built under
    // the lock. The object is constructed by the caller.
    AllocationContext reserve( HashType type );
    // True if the slot still holds the allocation of this key (checked under the lock).
    bool          alive( const AllocationKey& key ) const;
    // Bumps the slot's generation and marks it free; called after the object was destroyed.
    void          release( const AllocationKey& key );
    void*         slot_ptr( const AllocationKey& key ) const;
    AllocationKey key_of( const void* ptr ) const;
    bool          is_safe( const void* ptr ) const;

    void          init();
    void          commit_chunk();
    AllocationKey get_free_space();
    bool          peak( const AllocationKey& key ) const;
    void          flip( const AllocationKey& key, bool allocate );

    size_t                    m_element_size_;
    size_t                    m_reserve_bytes_;
    size_t                    m_capacity_chunks_    = 0;       // 256-slot chunks that fit in the reservation
    size_t                    m_reallocation_count_ = 0;       // storage never moves; kept for the context check
    char*                     m_start_ptr_          = nullptr; // reserved object range
    __m256i*                  m_mask_               = nullptr; // reserved, one entry per chunk
    size_t*                   m_local_allocation_count_ = nullptr; // reserved, one generation per slot
    std::atomic<size_t>       m_chunks_             = 0;       // committed chunks
    std::atomic<ownership>    m_ownership_          = ownership::undecided;
    mutable std::atomic<bool> m_lock_               = false;
    bool                      m_initialized_        = false;
};

struct ENGINE_MEMORY_API AllocationContext
{
private:
    AllocationKey              m_key_                     = null_pair;
    mutable size_t             m_type_reallocation_count_ = -1;
    const pool_core*           m_allocator_               = nullptr;
    HashType                   m_allocator_type_          = nullptr;
    mutable void*              m_ptr_				      = nullptr;

	template <typename T>
    friend class object_pool_allocator;

    friend class pool_core;

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
        m_allocator_type_          = context.m_allocator_type_;
        m_ptr_                     = context.m_ptr_;
	}

	static const AllocationContext& get_null_context()
	{
        static AllocationContext null_context( no_assert_tag{} );
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
        return m_allocator_->past( *this ) || m_ptr_ == nullptr;
	}

	void update() const
	{
        m_type_reallocation_count_ = m_allocator_->allocation_count();
        m_ptr_                     = m_allocator_->get_ptr( *this );
	}

	bool expired() const
    {
		if ( !valid() )
		{
		    return false;
		}

		if ( should_resolve() )
		{
            update();
		}

        return valid() && m_ptr_ == nullptr;
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

	AllocationContext( const AllocationKey& key, const pool_core* alloc, const HashType type )
	{
        assert( key != null_pair && alloc );
        m_key_                     = key;
        m_allocator_               = alloc;
        m_allocator_type_          = type;
		m_type_reallocation_count_ = m_allocator_->allocation_count();
        m_ptr_                     = m_allocator_->get_ptr( *this );
	}

	size_t type_allocation_count() const
	{
        return m_type_reallocation_count_;
	}

	size_t allocation_count() const
	{
        return m_key_.allocation_count;
	}

	size_t index() const
	{
        return m_key_.to_raw_index();
	}

	const pool_core* allocator() const
	{
        return m_allocator_;
	}

	HashType allocator_type() const 
	{
		return m_allocator_type_;
	}

	bool owner_before(const AllocationContext& other) const
	{
		if ( allocator() != other.allocator() )
		{
            return reinterpret_cast<uintptr_t>( allocator() ) < reinterpret_cast<uintptr_t>( other.allocator() );
		}

		if ( index() == other.index() )
		{
            return allocation_count() < other.allocation_count();
		}

        return index() < other.index();
	}

	bool owner_equals(const AllocationContext& other) const
	{
        return index() == other.index() &&
               allocation_count() == other.allocation_count() &&
               allocator() == other.allocator();
	}

	size_t owner_hash_code() const
	{
        static std::hash<size_t> hasher;
		static std::hash<uintptr_t> ptr_hasher;
        size_t value = m_key_.to_raw_index();
        boost::hash_combine( value, hasher( allocation_count() ) );
        boost::hash_combine( value, ptr_hasher( reinterpret_cast<uintptr_t>( allocator() ) ) );
		return value;
	}
};

template <typename T>
class object_pool_allocator;

// Address space reserved for the objects of a type's pool (not memory: pages are committed as the pool grows).
// Specialize for a type that needs more; the specialization must be visible wherever the type's pool is first used
// (the first module to use the type decides, and other modules must agree).
template <typename T>
inline constexpr size_t pool_reserve_bytes = size_t( 1 ) << 30;

class ENGINE_MEMORY_API PoolAllocatorStorage
{
private:
    // Keyed by type value, so every DLL gets the same pool_core for a type. The key points at the type_hash value of
    // the module that registered the type first.
    // Pool cores are created here (Memory.dll) and never destroyed, see pool_core.
    std::unordered_map<HashType, pool_core*, HashTypeHash> m_allocators_;
    mutable std::mutex                                                m_allocators_mutex_;
    std::atomic<bool>                                                 m_closed_ = false; // set by cleanup()

    // The pool core of a type, created on first use. Same instance for every DLL (keyed by type value). Reached only
    // through get_allocator, so pool cores are not handed out.
    pool_core* get_core( HashType type, size_t element_size, size_t reserve_bytes );

public:
    // The typed front of a type's pool. A per-DLL object that only refers to the shared pool_core.
    template <typename T>
    object_pool_allocator<T>& get_allocator()
    {
        static object_pool_allocator<T> allocator( get_core( &type_hash<T>::value, sizeof( T ), pool_reserve_bytes<T> ) );
        return allocator;
    }

	// Called on every managed pointer access, so no lock: a valid allocation context always refers to a registered
	// allocator, and entries are only removed all at once by cleanup().
	bool is_allocator_live( HashType /*type*/ ) const
	{
        return !m_closed_.load( std::memory_order_acquire );
	}

    void        cleanup();
    static void report_leakage();
};

extern ENGINE_MEMORY_API PoolAllocatorStorage g_allocator_storage;

#if defined( _DEBUG ) && Platform == Windows
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <iostream>
#include <stdlib.h>

struct debug_new_delete
{
    typedef std::size_t size_type;
    //!< An unsigned integral type that can represent the size of the largest object to be allocated.
    typedef std::ptrdiff_t difference_type;
    //!< A signed integral type that can represent the difference of any two pointers.

    static char* malloc( const size_type bytes )
    {
        //! Attempts to allocate n bytes from the system. Returns 0 if out-of-memory
#define DBG_NEW new ( _NORMAL_BLOCK, __FILE__, __LINE__ )
        // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
        // allocations to be of _CLIENT_BLOCK type
        return DBG_NEW char[ bytes ];
#undef DBG_NEW
    }

    static void free( char* const block )
    {
        //! Attempts to de-allocate block.
        //! \pre Block must have been previously returned from a call to UserAllocator::malloc.
        delete[] block;
    }
};
using user_allocator = debug_new_delete;
#else
using user_allocator = boost::default_user_allocator_new_delete;
#endif

struct managed_pool_access;

// Typed front of a pool: constructs and destroys T in slots of the shared pool_core. Instances are per-DLL and
// hold no state of their own, so allocation contexts refer to the pool_core, not to this object.
//
// Objects owned by managed pointers are created only by make_managed_shared and freed only by their control block
// (through managed_pool_access), so a managed pointer's object cannot be freed under it. Objects without managed
// pointers (e.g. UI tokens) use allocate_unmanaged/deallocate_unmanaged; a pool serves one kind or the other.
template <typename T>
class object_pool_allocator
{
    friend struct managed_pool_access;

public:
    explicit object_pool_allocator( pool_core* core )
        : m_core_( core )
    { }

    // The caller owns the object and frees it with deallocate_unmanaged.
    template <typename... Args>
    T* allocate_unmanaged( Args&&... arguments )
    {
        m_core_->claim_ownership( pool_core::ownership::unmanaged );
        return static_cast<T*>( m_core_->slot_ptr( allocate( std::forward<Args>( arguments )... ).m_key_ ) );
    }

    // Frees an object from allocate_unmanaged. Nothing after cleanup (like the managed pointers' deleter).
    void deallocate_unmanaged( T* ptr )
    {
        if ( !g_allocator_storage.is_allocator_live( &type_hash<T>::value ) )
        {
            return;
        }

        if ( m_core_->current_ownership() != pool_core::ownership::unmanaged || !m_core_->is_safe( ptr ) )
        {
            pool_core::fail_fast( "object pool: deallocate_unmanaged of an object it does not own" );
        }

        // Must be the start of a slot that holds an object (not an interior pointer or a slot already freed).
        const AllocationKey key = m_core_->key_of( ptr );
        if ( m_core_->slot_ptr( key ) != ptr || !m_core_->alive( key ) )
        {
            pool_core::fail_fast( "object pool: deallocate_unmanaged of an object it does not own" );
        }

        deallocate( get_context( ptr ) );
    }

    // Null context for an object that is not in this pool (e.g. created on the heap by deserialization).
    AllocationContext get_context( const T* ptr ) const
    {
        if ( !m_core_->is_safe( ptr ) )
        {
            return AllocationContext::get_null_context();
        }

        return AllocationContext( m_core_->key_of( ptr ), m_core_, &type_hash<T>::value );
    }

    size_t allocation_count() const
    {
        return m_core_->allocation_count();
    }

private:
    template <typename... Args>
    AllocationContext allocate( Args&&... arguments )
    {
        const AllocationContext context = m_core_->reserve( &type_hash<T>::value );
        ConstructorAccess::InternalConstruct<T>( static_cast<T*>( m_core_->slot_ptr( context.m_key_ ) ),
                                                 std::forward<Args>( arguments )... );

        return context;
    }

    // Frees the object in the context's slot if it is still the same generation; a no-op otherwise (already freed,
    // or the slot was reused), so a second call for the same allocation is harmless.
    // The destructor runs outside the spinlock: it may release other objects of the same type.
    static void deallocate( const AllocationContext& context )
    {
        if ( !context.valid() )
        {
            return;
        }

        const pool_core* core = context.allocator();
        if ( !core->alive( context.m_key_ ) )
        {
            return;
        }

        T* ptr = static_cast<T*>( core->slot_ptr( context.m_key_ ) );
        ConstructorAccess::InternalPreDeconstruction( ptr );
        ConstructorAccess::InternalDeconstruct( ptr );

        const_cast<pool_core*>( core )->release( context.m_key_ );
    }

    pool_core* m_core_;
};

template <typename T>
struct managed_slot_deleter;

// The managed side of object_pool_allocator: allocation for make_managed_shared and freeing for the control
// block's deleter, which are the only ones that can use it.
struct managed_pool_access
{
private:
    template <typename T>
    friend struct managed_slot_deleter;

    template <typename T, typename... Args>
    friend managed_shared_ptr<T> make_managed_shared( Args&&... arguments );

    template <typename T, typename... Args>
    static AllocationContext allocate( Args&&... arguments )
    {
        object_pool_allocator<T>& alloc = g_allocator_storage.get_allocator<T>();
        alloc.m_core_->claim_ownership( pool_core::ownership::managed );
        return alloc.allocate( std::forward<Args>( arguments )... );
    }

    template <typename T>
    static void deallocate( const AllocationContext& context )
    {
        object_pool_allocator<T>::deallocate( context );
    }
};

// Deleter of the one boost control block that owns a pooled object. It finds the object by allocation slot and
// generation instead of the raw pointer.
// T is the allocated (most derived) type: the destructor is called from the module that created the control block,
// the same module whose code the control block's vtable already belongs to.
template <typename T>
struct managed_slot_deleter
{
    AllocationContext context;

    void operator()( const void* /*stale address*/ ) const noexcept
    {
        // if there is no instanced allocator, it is undefined behaviour so dismiss the calling deallocation.
        if ( g_allocator_storage.is_allocator_live( context.allocator_type() ) )
        {
            managed_pool_access::deallocate<T>( context );
        }
    }
};

namespace Engine
{
    struct Serializer;
}

template <typename T>
class managed_shared_ptr : protected boost::shared_ptr<T>
{
    AllocationContext m_context_;

	template <typename U> 
	friend class managed_weak_ptr;
    
	template <typename U>
    friend class managed_shared_ptr;

    friend struct std::hash<managed_shared_ptr>;

	friend struct Engine::Serializer;

	friend class boost::serialization::access;
	
    template <typename T1, typename T2>
    friend managed_shared_ptr<T1> managed_reinterpret_pointer_cast( const managed_shared_ptr<T2>& r ) noexcept;

    template <typename T1, typename T2>
    friend managed_shared_ptr<T1> managed_static_pointer_cast( const managed_shared_ptr<T2>& r ) noexcept;

	template <typename T, typename... Args>
    friend managed_shared_ptr<T> make_managed_shared( Args&&... arguments );

    template <typename T, typename U, typename... Args>
    friend managed_shared_ptr<U> make_managed_shared( Args&&... arguments );

	template <typename T>
    friend managed_shared_ptr<T> from_native( const boost::weak_ptr<T>& native, T* self );

	template <typename T>
    friend managed_shared_ptr<T> from_native( const boost::shared_ptr<T>& native );

	struct cast_tag
    { };

	struct init_tag {};

	template <typename U>
    managed_shared_ptr( const managed_shared_ptr<U>& other, const cast_tag& )
        : boost::shared_ptr<T>( boost::reinterpret_pointer_cast<T>( other ) ), m_context_( other.m_context_ )
	{
	}

	// The only place a control block is created: one block per allocation, shared by every copy.
	managed_shared_ptr( const AllocationContext& context, const init_tag& )
        : boost::shared_ptr<T>( static_cast<T*>( context.ptr() ), managed_slot_deleter<T>{ context } ), m_context_( context )
    {
	}

	template <typename U>
    explicit managed_shared_ptr( const boost::weak_ptr<U>& weak_native, const AllocationContext& context )
        : boost::shared_ptr<T>( weak_native, boost::detail::sp_nothrow_tag{} ), m_context_( context )
    {
        drop_if_unmanaged();
    }

	template <typename U>
    explicit managed_shared_ptr( const boost::shared_ptr<U>& native, const AllocationContext& context )
        : boost::shared_ptr<T>( native ), m_context_( context )
    {
        drop_if_unmanaged();
    }

	template <typename U>
    explicit managed_shared_ptr( const managed_weak_ptr<U>& weak_native )
        : boost::shared_ptr<T>( weak_native, boost::detail::sp_nothrow_tag{} ), m_context_( weak_native.m_context_ )
    {
        drop_if_unmanaged();
    }

	// Decided once where a pointer can be made from something other than an allocation or another managed pointer,
	// with the result resolve() used to give on every access:
	// - an object without a pool allocation (e.g. created on the heap by deserialization) reads as null;
	// - a pointer to an object that is gone (e.g. a weak pointer locked after the object died) is empty, with a null
	//   context, so hashing, ordering and to_weak() treat it as empty.
	void drop_if_unmanaged()
	{
		if ( !g_allocator_storage.is_allocator_live( m_context_.allocator_type() ) )
		{
			return;
		}

		// No object (e.g. a weak pointer locked while its object is being destroyed): empty, so operator bool agrees
		// with get().
		if ( !boost::shared_ptr<T>::get() )
		{
			m_context_ = AllocationContext::get_null_context();
			return;
		}

		if ( !m_context_.valid() )
		{
			boost::shared_ptr<T>::reset();
			return;
		}

		if ( m_context_.expired() )
		{
			boost::shared_ptr<T>::reset();
			m_context_ = AllocationContext::get_null_context();
		}
	}

	AllocationContext internal_context() const
    {
        return m_context_;
    }

    [[nodiscard]] boost::shared_ptr<T> native() const
    {
        return *this;
    }

	template <class Archive>
    void serialize( Archive& ar, const unsigned int file_version )
    {
        if constexpr ( Archive::is_loading::value )
        {
            // A loaded object is not the allocation this pointer may have held before.
            m_context_ = AllocationContext::get_null_context();
        }
        ar & static_cast<boost::shared_ptr<T>&>( *this );
        if constexpr ( Archive::is_loading::value )
        {
            drop_if_unmanaged();
        }
	}

public:
    using element_type = T;

	// Declared so that no implicit move is generated (moves copy): a moved-from boost pointer would keep a live context.
	~managed_shared_ptr() = default;

	managed_shared_ptr( nullptr_t )
        : boost::shared_ptr<T>( nullptr ), m_context_( AllocationContext::get_null_context()  )
    {
	}

	managed_shared_ptr()
        : boost::shared_ptr<T>( nullptr ), m_context_( AllocationContext::get_null_context()  )
	{
	}

	template <typename U>
     managed_shared_ptr( const managed_shared_ptr<U>& other )
        : boost::shared_ptr<T>( boost::static_pointer_cast<T>( other.native() ) ), m_context_( other.m_context_ )
    {
    }

	// After cleanup every managed pointer reads as null, like get().
	explicit operator bool() const noexcept
    {
        if ( !g_allocator_storage.is_allocator_live( m_context_.allocator_type() ) || !m_context_.valid() )
        {
            return false;
        }
        return !m_context_.expired();
	}

	bool operator==( nullptr_t ) const
    {
        return !g_allocator_storage.is_allocator_live( m_context_.allocator_type() ) || !m_context_.valid() ||
               m_context_.expired();
	}

	operator boost::shared_ptr<T>() const
	{
        boost::shared_ptr<T> result( *this );
        return result;
	}

	void reset()
	{
        // Dropping the share is enough: the control block's deleter frees the object when this was the last owner.
        *this = managed_shared_ptr<T>();
	}

	// The stored pointer is set when the pointer is made and does not change: pools never move their objects, and
	// the object stays alive while this owns a share. Null after cleanup.
	T* resolve() const noexcept
    {
		if ( !g_allocator_storage.is_allocator_live( m_context_.allocator_type() ) )
		{
			return nullptr;
		}

		T* const stored = boost::shared_ptr<T>::get();
		// The allocation must still be the one this pointer owns. It cannot change while a share is held; if it did
		// (a bug elsewhere), stop instead of handing out a destroyed or reused object. Covers get(), -> and * only:
		// raw pointers kept from get(), operator bool and weak pointers are not checked here.
		if ( stored && !m_context_.allocator()->is_current( m_context_.m_key_ ) )
		{
			pool_core::fail_fast( "managed_shared_ptr: the object was freed while a managed pointer owned it" );
		}
		return stored;
	}

	[[nodiscard]] size_t use_count() const
	{
        return boost::shared_ptr<T>::use_count();
	}

	bool operator!() const noexcept
    {
        return !this->operator bool();
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

	bool expired() const noexcept
	{
        return m_context_.expired();
	}

	template <typename U>
	bool owner_before( const managed_shared_ptr<U>& other ) const
	{
        return m_context_.owner_before( other.m_context_ );
	}

	template <typename U>
	bool owner_before( const managed_weak_ptr<T>& other ) const
	{
        return m_context_.owner_before( other.m_context_ );
	}

	template <typename U>
    bool owner_equals( const managed_shared_ptr<U>& other ) const
    {
        return m_context_.owner_equals( other.m_context_ );
    }

	template <typename U>
    bool owner_equals( const managed_weak_ptr<U>& other ) const
    {
        return m_context_.owner_equals( other.m_context_ );
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
    static typename managed_shared_ptr<T>::cast_tag placeholder{};
    return managed_shared_ptr<T>( r, placeholder );
}

template <typename T, typename U>
managed_shared_ptr<T> managed_static_pointer_cast( const managed_shared_ptr<U>& r ) noexcept
{
    static typename managed_shared_ptr<T>::cast_tag placeholder{};
    ( void )static_cast<T*>( static_cast<U*>( nullptr ) );

    return managed_shared_ptr<T>( r, placeholder );
}

template <typename T>
class managed_weak_ptr : protected boost::weak_ptr<T>
{
    AllocationContext m_context_;

	template <typename U>
    friend class managed_weak_ptr;

	template <typename U>
	friend class managed_shared_ptr;

    friend struct std::hash<managed_weak_ptr<T>>;

	struct init_tag
    { };

	managed_weak_ptr( const AllocationContext& context, const init_tag& )
        : boost::weak_ptr<T>( nullptr ), m_context_( context )
    {
    }

public:
    managed_weak_ptr() : boost::weak_ptr<T>( {} ), m_context_( AllocationContext::no_assert_tag{} )
    {
        m_context_ = AllocationContext::get_null_context();
    }

	template <typename U>
	managed_weak_ptr( const managed_weak_ptr<U>& other )
		: boost::weak_ptr<T>( other ),
		m_context_( other.m_context_ )
	{ }

    managed_weak_ptr( const managed_shared_ptr<T>& other )
        : boost::weak_ptr<T>( other ), m_context_( other.m_context_ )
    { }

	template <typename U>
    managed_weak_ptr( const managed_shared_ptr<U>& other )
        : boost::weak_ptr<T>( other ), m_context_( other.m_context_ )
    { }

	void reset()
    {
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
        return !m_context_.valid();
	}

	template <typename U>
    [[nodiscard]] bool owner_before( const managed_weak_ptr<U>& other ) const
    {
        return m_context_.owner_before( other.m_context_ );
    }

	template <typename U>
    [[nodiscard]] bool owner_before( const managed_shared_ptr<U>& other ) const
    {
        return m_context_.owner_before( other.m_context_ );
    }

	template <typename U>
    [[nodiscard]] bool owner_equals( const managed_weak_ptr<U>& other ) const
    {
        return m_context_.owner_equals( other.m_context_ );
    }

	template <typename U>
    [[nodiscard]] bool owner_equals( const managed_shared_ptr<U>& other ) const
    {
        return m_context_.owner_equals( other.m_context_ );
    }


	managed_shared_ptr<T> lock() const
    {
		return managed_shared_ptr<T>( *this );
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
        return p.m_context_.owner_hash_code();
    }
};

template <typename T>
struct std::hash<managed_weak_ptr<T>>
{
    size_t operator()( const managed_weak_ptr<T>& p ) const noexcept
    {
        return p.m_context_.owner_hash_code();
    }
};

template <typename T, typename U>
inline bool operator<( const managed_shared_ptr<T>& left, const managed_shared_ptr<U>& right ) noexcept
{
    return left.owner_before( right );
}

template <typename T, typename U>
inline bool operator<( const managed_weak_ptr<T>& left, const managed_weak_ptr<U>& right ) noexcept
{
    return left.owner_before( right );
}

template <typename T, typename... Args>
managed_shared_ptr<T> make_managed_shared( Args&&... arguments )
{
    static typename managed_shared_ptr<T>::init_tag tag{};
    return managed_shared_ptr<T>( managed_pool_access::allocate<T>( std::forward<Args>( arguments )... ), tag );
}

template <typename T, typename U, typename... Args>
managed_shared_ptr<U> make_managed_shared( Args&&... arguments )
{
    // Created as T, so the control block's deleter destroys the allocated type, then converted to U.
    return managed_shared_ptr<U>( make_managed_shared<T>( std::forward<Args>( arguments )... ) );
}

// `self` is the object (weak_from_this() of it); pools never move objects, so the weak pointer already holds its
// address.
template <typename T>
managed_shared_ptr<T> from_native( const boost::weak_ptr<T>& native, T* self )
{
    if ( const boost::shared_ptr<T> locked = native.lock() )
	{
        assert( locked.get() == self );
        return managed_shared_ptr<T>( locked, self->GetAllocationContext() );
	}

	return {};
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


template <typename T, typename BoundFunctionT, typename... Args>
struct managed_binder
{
private:
    BoundFunctionT func;
    managed_weak_ptr<T>   ptr;
	std::tuple<std::decay_t<Args>...> args;

	using Indices = std::make_index_sequence<sizeof...(Args)>;

	template <size_t... N>
	void invoke_proxy_impl( std::index_sequence<N...>, const BoundFunctionT& func, T* ptr ) const noexcept
	{
        std::bind( func, ptr, std::get<N>( args )... )();
	}

    void invoke_proxy( const BoundFunctionT& func, T* ptr ) const noexcept
	{
        invoke_proxy_impl( Indices{ }, func, ptr );
	}

public:
    managed_binder( BoundFunctionT func, managed_weak_ptr<T> ptr, Args&&... args )
        : func( func ), ptr( ptr ), args( std::forward_as_tuple( args... ) )
    { }

	~managed_binder()
	{
	}

	void operator()() const noexcept
	{
		if ( const managed_shared_ptr<T>& locked = ptr.lock() )
		{
            invoke_proxy( func, locked.get() );
		}
	}
};

template <typename FunctionT, typename T, typename... Args>
auto managed_bind(FunctionT function, managed_weak_ptr<T> ptr, Args&&... args)
{
    return managed_binder<T, FunctionT, Args...>( function, ptr, args... );
}

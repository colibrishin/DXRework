#pragma once
#include <map>
#include <queue>
#include <set>
#include "Source/Runtime/TypeLibrary/Public/TypeLibrary.h"

#include <boost/align/aligned_allocator.hpp>
#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_vector.h>

namespace Engine
{
	constexpr uint64_t Align(uint64_t size, uint64_t alignment)
	{
		return (size + alignment - 1) & ~(alignment - 1);
	}

	consteval size_t nearest_pow_two(size_t value)
	{
		size_t result = 1;

		while (value > result)
		{
			result = result << 1;
		}

		return result;
	}

	inline static constexpr size_t g_default_pool_size = 16;
	inline static constexpr size_t g_cache_alignment = 16;

	template <typename KeyType, typename ValueType>
	struct aligned_pair_size
	{
		constexpr unsigned operator()() const
		{
			return static_cast<unsigned>(Align(sizeof(std::pair<const KeyType, ValueType>) * g_default_pool_size, g_cache_alignment));
		}
	};

	template <typename KeyType, typename ValueType>
	using u_fast_pool_allocator = boost::fast_pool_allocator<std::pair<const KeyType, ValueType>>;

	template <typename ValueType>
	using u_fast_pool_allocator_single = boost::fast_pool_allocator<ValueType>;

	template <typename ValueType>
	using u_align_allocator = boost::alignment::aligned_allocator<ValueType, nearest_pow_two(sizeof(ValueType))>;

	template <typename ValueType>
	using u_pool_allocator_single = boost::pool_allocator<ValueType>;

	template <typename KeyType>
	using fast_pool_set = std::set<KeyType, std::less<KeyType>, u_fast_pool_allocator_single<KeyType>>;

	template <typename KeyType, typename ValueType>
	using fast_pool_unordered_map = std::unordered_map<KeyType, ValueType, std::hash<KeyType>, std::equal_to<KeyType>, u_fast_pool_allocator<KeyType, ValueType>>;

	template <typename KeyType, typename ValueType>
	using fast_pool_map = std::map<KeyType, ValueType, std::less<KeyType>, u_fast_pool_allocator<KeyType, ValueType>>;

	template <typename ValueType>
	using pool_queue = std::queue<ValueType, std::deque<ValueType, u_pool_allocator_single<ValueType>>>;

	template <typename ValueType>
	using aligned_vector = std::vector<ValueType, u_align_allocator<ValueType>>;

	template <typename KeyType, typename ValueType>
	using concurrent_fast_pool_map = tbb::concurrent_hash_map<KeyType, ValueType, tbb::tbb_hash_compare<KeyType>, u_fast_pool_allocator<KeyType, ValueType>>;

	template <typename ValueType>
	using concurrent_aligned_vector = tbb::concurrent_vector<ValueType, u_align_allocator<ValueType>>;

	// Concurrent type definitions
	using ConcurrentWeakObjGlobalMap = concurrent_fast_pool_map<GlobalEntityID, Weak<Abstracts::ObjectBase>>;
	using ConcurrentWeakObjVec = tbb::concurrent_vector<Weak<Abstracts::ObjectBase>, u_pool_allocator_single<Weak<Abstracts::ObjectBase>>>;
	using ConcurrentLocalGlobalIDMap = concurrent_fast_pool_map<LocalActorID, GlobalEntityID>;
	using ConcurrentWeakComVec = tbb::concurrent_vector<Weak<Abstracts::Component>, u_fast_pool_allocator_single<Weak<Abstracts::Component>>>;
	using ConcurrentWeakComMap = concurrent_fast_pool_map<GlobalEntityID, Weak<Abstracts::Component>>;
	using ConcurrentWeakScpVec = tbb::concurrent_vector<Weak<Script>, u_pool_allocator_single<Weak<Script>>>;
	using ConcurrentWeakScpMap = concurrent_fast_pool_map<GlobalEntityID, Weak<Script>>;
	using ConcurrentWeakComRootMap = concurrent_fast_pool_map<eComponentType, ConcurrentWeakComMap>;
	using ConcurrentWeakScpRootMap = concurrent_fast_pool_map<ScriptSizeType, ConcurrentWeakScpMap>;
}
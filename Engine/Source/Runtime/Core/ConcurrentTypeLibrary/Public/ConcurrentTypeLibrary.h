#pragma once
#include <map>
#include <queue>
#include <set>
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"

#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_vector.h>

namespace Engine
{
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

	using InstanceBufferContainer = tbb::concurrent_vector<Unique<IStructuredBufferType<Graphics::SBs::InstanceSB>>>;
}

#pragma once
#include <map>
#include <queue>
#include <set>
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/GraphicInterface.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"

#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_vector.h>


namespace Engine
{
	template <typename KeyType, typename ValueType>
	using concurrent_fast_pool_map = tbb::concurrent_hash_map<KeyType, ValueType, tbb::tbb_hash_compare<KeyType>, boost::fast_pool_allocator<std::pair<const KeyType, ValueType>, boost::default_user_allocator_new_delete, boost::details::pool::default_mutex, 64>>;

	template <typename ValueType>
	using concurrent_aligned_vector = tbb::concurrent_vector<ValueType, boost::alignment::aligned_allocator<ValueType, 64>>;

	// Concurrent type definitions
	using ConcurrentWeakObjGlobalMap = concurrent_fast_pool_map<GlobalEntityID, Weak<Abstracts::ObjectBase>>;
	using ConcurrentWeakObjVec = tbb::concurrent_vector<Weak<Abstracts::ObjectBase>, u_pool_allocator_single<Weak<Abstracts::ObjectBase>>>;
	using ConcurrentLocalGlobalIDMap = concurrent_fast_pool_map<LocalActorID, GlobalEntityID>;
	using ConcurrentWeakComVec = tbb::concurrent_vector<Weak<Abstracts::Component>, u_fast_pool_allocator_single<Weak<Abstracts::Component>>>;
	using ConcurrentWeakComMap = concurrent_fast_pool_map<GlobalEntityID, Weak<Abstracts::Component>>;
	using ConcurrentWeakScpVec = tbb::concurrent_vector<Weak<Script>, u_pool_allocator_single<Weak<Script>>>;
	using ConcurrentWeakScpMap = concurrent_fast_pool_map<GlobalEntityID, Weak<Script>>;
	using ConcurrentWeakComRootMap = concurrent_fast_pool_map<ComponentType, ConcurrentWeakComMap>;
	using ConcurrentWeakScpRootMap = concurrent_fast_pool_map<ScriptType, ConcurrentWeakScpMap>;

	using ConcurrentInstanceBufferContainer = tbb::concurrent_vector<StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>>;
}

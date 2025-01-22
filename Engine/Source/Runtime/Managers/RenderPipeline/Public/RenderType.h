#pragma once
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/StructuredBuffer.h"

namespace Engine 
{
	using CandidateTuple = std::tuple<Weak<Abstracts::ObjectBase>, Weak<Resources::Material>, aligned_vector<Graphics::SBs::InstanceSB>>;
	using RenderMapValueType = tbb::concurrent_vector<CandidateTuple, u_align_allocator<CandidateTuple>>;
	using RenderMap = tbb::concurrent_hash_map<HashType, RenderMapValueType>;
}
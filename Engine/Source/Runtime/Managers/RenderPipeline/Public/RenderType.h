#pragma once
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/StructuredBuffer/Public/StructuredBuffer.h"

namespace Engine 
{
	using CandidatePair = std::pair<Weak<Abstracts::ObjectBase>, aligned_vector<Graphics::SBs::InstanceSB>>;
	using RenderMapValueType = concurrent_fast_pool_map<Weak<Resources::Material>, CandidatePair>;
	using RenderMap = concurrent_fast_pool_map<HashType, RenderMapValueType>;
}
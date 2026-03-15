#pragma once
#include "Allocator.h"
#include "TypeLibrary.h"
#include "ConcurrentTypeLibrary.h"
#include "InstancePair.h"
#include "Object.h"

namespace Engine
{
	using MeshMap = concurrent_fast_pool_map<Strong<Abstracts::Resource>, aligned_vector<InstancePair>>;
	using ShaderMap = concurrent_fast_pool_map<Strong<Abstracts::Resource>, MeshMap>;
	// Object + Materials -> Mesh -> Shader -> Renderer -> Shader Domain
	using RenderMap = concurrent_fast_pool_map<HashType, ShaderMap>;
}

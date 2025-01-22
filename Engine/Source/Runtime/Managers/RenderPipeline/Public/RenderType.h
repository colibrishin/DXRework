#pragma once
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"

namespace Engine 
{
	struct InstancePair
	{
		Strong<Abstracts::ObjectBase> object;
		Graphics::SBs::InstanceSB* instance;
		std::array<Strong<Resources::Texture>, g_max_texture_per_material> textures;
		std::array<Strong<Resources::Texture>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN> reservedTextures;
	};

	using ShaderMap = concurrent_fast_pool_map<Strong<Resources::Shader>, aligned_vector<InstancePair>>;
	using MeshMap   = concurrent_fast_pool_map<Strong<Resources::Mesh>, ShaderMap>;
	// Object + Materials -> Shader -> Mesh -> Renderer -> Shader Domain
	using RenderMap = concurrent_fast_pool_map<HashType, MeshMap>;
}
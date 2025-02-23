#pragma once
#include "Allocator.h"
#include "TypeLibrary.h"
#include "ConcurrentTypeLibrary.h"

namespace Engine 
{
	struct InstancePair
	{
		Strong<Abstracts::ObjectBase> object;
		Graphics::SBs::InstanceSB* instance;
		std::array<Strong<Resources::Texture>, g_max_texture_per_material> textures;
		std::array<Strong<Resources::Texture>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN> reservedTextures;
	};

	using MeshMap = concurrent_fast_pool_map<Strong<Resources::Mesh>, aligned_vector<InstancePair>>;
	using ShaderMap = concurrent_fast_pool_map<Strong<Resources::ShaderBase>, MeshMap>;
	// Object + Materials -> Mesh -> Shader -> Renderer -> Shader Domain
	using RenderMap = concurrent_fast_pool_map<HashType, ShaderMap>;

    struct TexturePair
    {
        TexturePair() = default;

        TexturePair( const std::array<Strong<Resources::Texture>, g_max_texture_per_material> *textures,
                     const std::array<Strong<Resources::Texture>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN>
                             *reservedTextures )
            : textures( textures ),
              reservedTextures( reservedTextures ),
              boundTextureCount( std::ranges::count_if(
                      *textures, []( const Strong<Resources::Texture> &tex ) { return tex != nullptr; } ) )
        {}

        const std::array<Strong<Resources::Texture>, g_max_texture_per_material>                      *textures;
        const std::array<Strong<Resources::Texture>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN> *reservedTextures;

        [[nodiscard]] size_t GetTextureCount() const
        {
            return boundTextureCount;
        }

    private:
        size_t boundTextureCount;
    };
}
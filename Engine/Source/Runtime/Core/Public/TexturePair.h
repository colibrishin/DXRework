#pragma once
#include "TypeLibrary.h"
#include "Texture.h"

namespace Engine
{
    struct TexturePair
    {
        TexturePair() = default;

        TexturePair( const std::array<Strong<Resources::Texture>, g_max_texture_per_material>* textures,
                     const std::array<Strong<Resources::Texture>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN>*
                             reservedTextures )
            : textures( textures ),
              reservedTextures( reservedTextures ),
              boundTextureCount( std::ranges::count_if(
                      *textures, []( const Strong<Resources::Texture>& tex ) { return tex != nullptr; } ) )
        { }

        const std::array<Strong<Resources::Texture>, g_max_texture_per_material>*                      textures;
        const std::array<Strong<Resources::Texture>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN>* reservedTextures;

        [[nodiscard]] size_t GetTextureCount() const
        {
            return boundTextureCount;
        }

    private:
        size_t boundTextureCount;
    };
}

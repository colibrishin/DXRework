#pragma once
#include "TypeLibrary.h"

namespace Engine
{
    struct TexturePair
    {
        TexturePair() = default;

        TexturePair( const std::array<Strong<Abstracts::Resource>, g_max_texture_per_material>* textures,
                     const std::array<Strong<Abstracts::Resource>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN>*
                             reservedTextures )
            : textures( textures ),
              reservedTextures( reservedTextures ),
              boundTextureCount( std::ranges::count_if(
                      *textures, []( const Strong<Abstracts::Resource>& tex ) { return tex != nullptr; } ) )
        { }

        const std::array<Strong<Abstracts::Resource>, g_max_texture_per_material>*                      textures;
        const std::array<Strong<Abstracts::Resource>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN>* reservedTextures;

        [[nodiscard]] size_t GetTextureCount() const
        {
            return boundTextureCount;
        }

    private:
        size_t boundTextureCount;
    };
}

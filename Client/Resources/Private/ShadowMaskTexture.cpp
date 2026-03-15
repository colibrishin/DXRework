#include "Resources/Public/ShadowMaskTexture.h"

ShadowMaskTexture::ShadowMaskTexture( const std::filesystem::path &path )
    : Texture2D( "",
        {
            .Dimension = Engine::TEX_TYPE_2D,
            .Alignment = 0,
            .Width = CFG_CASCADE_SHADOW_TEX_WIDTH,
            .Height = CFG_CASCADE_SHADOW_TEX_HEIGHT,
            .DepthOrArraySize = CFG_CASCADE_SHADOW_COUNT,
            .Format = Engine::TEX_FORMAT_R8G8B8A8_UNORM,
            .Flags = Engine::RESOURCE_FLAG_ALLOW_RENDER_TARGET,
            .MipsLevel = 1,
            .Layout = Engine::TEX_LAYOUT_UNKNOWN,
            .SampleDesc = {1, 0},
            .AsSRV = true,
            .AsRTV = true
        } )
{}

ShadowMaskTexture::ShadowMaskTexture()
    : Texture2D( "",
                 { .Dimension        = Engine::TEX_TYPE_2D,
                   .Alignment        = 0,
                   .Width            = CFG_CASCADE_SHADOW_TEX_WIDTH,
                   .Height           = CFG_CASCADE_SHADOW_TEX_HEIGHT,
                   .DepthOrArraySize = CFG_CASCADE_SHADOW_COUNT,
                   .Format           = Engine::TEX_FORMAT_R8G8B8A8_UNORM,
                   .Flags            = Engine::RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                   .MipsLevel        = 1,
                   .Layout           = Engine::TEX_LAYOUT_UNKNOWN,
                   .SampleDesc       = { 1, 0 },
                   .AsSRV            = true,
                   .AsRTV            = true } )
{}

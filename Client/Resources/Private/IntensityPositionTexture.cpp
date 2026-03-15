#include "Resources/Public/IntensityPositionTexture.h"

IntensityPositionTexture::IntensityPositionTexture( const std::filesystem::path &path )
    : Texture2D( "",
    {
        .Dimension = Engine::TEX_TYPE_2D,
        .Alignment = 0,
        .Width = CFG_CASCADE_SHADOW_TEX_WIDTH,
        .Height = CFG_CASCADE_SHADOW_TEX_HEIGHT,
        .DepthOrArraySize = 1,
        .Format = Engine::TEX_FORMAT_R32G32B32A32_FLOAT,
        .Flags = Engine::RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        .MipsLevel = 1,
        .Layout = Engine::TEX_LAYOUT_UNKNOWN,
        .SampleDesc = {1, 0},
        .AsSRV = true,
        .AsRTV = true
    })
{}

IntensityPositionTexture::IntensityPositionTexture()
    : Texture2D( "",
                 { .Dimension        = Engine::TEX_TYPE_2D,
                   .Alignment        = 0,
                   .Width            = CFG_CASCADE_SHADOW_TEX_WIDTH,
                   .Height           = CFG_CASCADE_SHADOW_TEX_HEIGHT,
                   .DepthOrArraySize = 1,
                   .Format           = Engine::TEX_FORMAT_R32G32B32A32_FLOAT,
                   .Flags            = Engine::RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                   .MipsLevel        = 1,
                   .Layout           = Engine::TEX_LAYOUT_UNKNOWN,
                   .SampleDesc       = { 1, 0 },
                   .AsSRV            = true,
                   .AsRTV            = true } )
{}

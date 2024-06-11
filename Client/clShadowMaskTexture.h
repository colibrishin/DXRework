#pragma once
#include <egTexture2D.h>

namespace Client::Resource
{
  class ShadowMaskTexture final : public Engine::Resources::Texture2D
  {
  public:
    ShadowMaskTexture()
      : Texture2D
      (
       "", {
         .Alignment = 0,
         .Width = g_max_shadow_map_size,
         .Height = g_max_shadow_map_size,
         .DepthOrArraySize = g_max_shadow_cascades,
         .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
         .Flags =  D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
         .MipsLevel = 1,
         .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
         .SampleDesc = {1, 0},
       }
      ) { }
  };
}

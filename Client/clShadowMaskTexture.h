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
         .Width = g_max_shadow_map_size,
         .Height = g_max_shadow_map_size,
         .Depth = 0,
         .ArraySize = g_max_shadow_cascades,
         .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
         .CPUAccessFlags = 0,
         .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
         .MipsLevel = 1,
         .MiscFlags = 0,
         .Usage = D3D11_USAGE_DEFAULT,
         .SampleDesc = {1, 0},
       }
      ) { }
  };
}

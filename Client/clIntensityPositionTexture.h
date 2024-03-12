#pragma once
#include <egTexture2D.h>

namespace Client::Resource
{
  class IntensityPositionTexture final : public Engine::Resources::Texture2D
  {
  public:
    IntensityPositionTexture()
      : Texture2D
      (
       "", {
         .Width = g_max_shadow_map_size,
         .Height = g_max_shadow_map_size,
         .Depth = 0,
         .ArraySize = 1,
         .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
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

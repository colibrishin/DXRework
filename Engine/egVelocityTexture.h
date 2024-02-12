#pragma once
#include "egGlobal.h"
#include "egTexture2D.h"

namespace Engine::Resources
{
  class VelocityTexture final : public Texture2D
  {
  public:
    VelocityTexture()
      : Texture2D("", {}) {}

    void loadDerived(ComPtr<ID3D11Resource>& res) override
    {
      LazyDescription
        (
         {
           .Width = g_window_width,
           .Height = g_window_height,
           .Depth = 0,
           .ArraySize = 1,
           .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
           .CPUAccessFlags = 0,
           .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
           .MipsLevel = 1,
           .MiscFlags = 0,
           .Usage = D3D11_USAGE_DEFAULT,
           .SampleDesc = {.Count = 1, .Quality = 0}
         }
        );

      Texture2D::loadDerived(res);
    }
  };
}

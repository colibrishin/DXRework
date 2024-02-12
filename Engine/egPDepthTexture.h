#pragma once
#include "egGlobal.h"
#include "egTexture2D.h"

namespace Engine::Resources
{
  class PDepthTexture final : public Texture2D
  {
  public:
    PDepthTexture()
      : Texture2D("", {
        .Width = g_window_width,
        .Height = g_window_height,
        .Depth = 0,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R24G8_TYPELESS,
        .CPUAccessFlags = 0,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL,
        .MipsLevel = 1,
        .MiscFlags = 0,
        .Usage = D3D11_USAGE_DEFAULT,
        .SampleDesc = {.Count = 1, .Quality = 0}
      }) {}

    void loadDerived(ComPtr<ID3D11Resource>& res) override
    {
      D3D11_DEPTH_STENCIL_VIEW_DESC   dsv_desc;
      D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

      dsv_desc.Format                         = DXGI_FORMAT_D24_UNORM_S8_UINT;
      dsv_desc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2D;
      dsv_desc.Flags                          = 0;
      dsv_desc.Texture2D.MipSlice             = 0;

      srv_desc.Format                         = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
      srv_desc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2D;
      srv_desc.Texture2D.MipLevels            = 1;
      srv_desc.Texture2D.MostDetailedMip      = 0;

      LazySRV(srv_desc);
      LazyDSV(dsv_desc);

      Texture2D::loadDerived(res);
    }
  };
}

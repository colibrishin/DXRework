#include "pch.h"
#include "egShadowTexture.h"

namespace Engine::Resources
{
  void ShadowTexture::FixedUpdate(const float& dt) { Texture2D::FixedUpdate(dt); }

  void ShadowTexture::Initialize() { Texture2D::Initialize(); }

  void ShadowTexture::PostRender(const float& dt) { Texture2D::PostRender(dt); }

  void ShadowTexture::PostUpdate(const float& dt) { Texture2D::PostUpdate(dt); }

  void ShadowTexture::PreRender(const float& dt) { Texture2D::PreRender(dt); }

  void ShadowTexture::PreUpdate(const float& dt) { Texture2D::PreUpdate(dt); }

  void ShadowTexture::Render(const float& dt) { Texture2D::Render(dt); }

  void ShadowTexture::Update(const float& dt) { Texture2D::Update(dt); }

  void ShadowTexture::OnDeserialized() { Texture2D::OnDeserialized(); }

  void ShadowTexture::OnImGui() { Texture2D::OnImGui(); }

  eResourceType ShadowTexture::GetResourceType() const { return RES_T_SHADOW_TEX; }

  UINT ShadowTexture::GetArraySize() const { return Texture2D::GetArraySize(); }

  UINT ShadowTexture::GetHeight() const { return Texture2D::GetHeight(); }

  UINT ShadowTexture::GetWidth() const { return Texture2D::GetWidth(); }

  void ShadowTexture::Clear() const
  {
    GetD3Device().GetContext()->ClearDepthStencilView(m_dsv_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
  }

  void ShadowTexture::loadDerived(ComPtr<ID3D11Resource>& res)
  {
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

    dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags = 0;
    dsv_desc.Texture2D.MipSlice = 0;
    dsv_desc.Texture2DArray.ArraySize = g_max_shadow_cascades;
    dsv_desc.Texture2DArray.FirstArraySlice = 0;

    srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srv_desc.Texture2DArray.ArraySize = g_max_shadow_cascades;
    srv_desc.Texture2DArray.FirstArraySlice = 0;
    srv_desc.Texture2DArray.MipLevels = 1;
    srv_desc.Texture2DArray.MostDetailedMip = 0;

    LazySRV(srv_desc);
    LazyDSV(dsv_desc);

    Texture2D::loadDerived(res);
  }

  void ShadowTexture::Unload_INTERNAL() { Texture2D::Unload_INTERNAL(); }
}

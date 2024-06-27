#include "pch.h"
#include "egShadowTexture.h"

namespace Engine::Resources
{
  void ShadowTexture::FixedUpdate(const float& dt) { Texture2D::FixedUpdate(dt); }

  void ShadowTexture::Initialize() { Texture2D::Initialize(); }

  void ShadowTexture::PostUpdate(const float& dt) { Texture2D::PostUpdate(dt); }

  void ShadowTexture::PreUpdate(const float& dt) { Texture2D::PreUpdate(dt); }

  void ShadowTexture::Update(const float& dt) { Texture2D::Update(dt); }

  void ShadowTexture::OnSerialized() { Texture2D::OnSerialized(); }

  void ShadowTexture::OnDeserialized() { Texture2D::OnDeserialized(); }

  void ShadowTexture::OnImGui() { Texture2D::OnImGui(); }

  eResourceType ShadowTexture::GetResourceType() const { return RES_T_SHADOW_TEX; }

  UINT ShadowTexture::GetDepth() const { return Texture2D::GetDepth(); }

  UINT ShadowTexture::GetHeight() const { return Texture2D::GetHeight(); }

  UINT64 ShadowTexture::GetWidth() const { return Texture2D::GetWidth(); }

  void ShadowTexture::Clear(ID3D12GraphicsCommandList1* cmd) const
  {
    const auto& dsv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (GetRawResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    cmd->ResourceBarrier(1, &dsv_trans);

    cmd->ClearDepthStencilView(
        m_dsv_->GetCPUDescriptorHandleForHeapStart(), 
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
        1.0f, 
        0, 
        0, 
        nullptr);

    const auto& dsv_trans_back = CD3DX12_RESOURCE_BARRIER::Transition
      (GetRawResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON);

    cmd->ResourceBarrier(1, &dsv_trans_back);
  }

  void ShadowTexture::loadDerived(ComPtr<ID3D12Resource>& res)
  {
    constexpr D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc
    {
      .Format = DXGI_FORMAT_D32_FLOAT,
      .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY,
      .Flags = D3D12_DSV_FLAG_NONE,
      .Texture2DArray = {
        .MipSlice = 0,
        .FirstArraySlice = 0,
        .ArraySize = g_max_shadow_cascades
      }
    };

    constexpr D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc
    {
      .Format = DXGI_FORMAT_R32_FLOAT,
      .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY,
      .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
      .Texture2DArray =
      {
        .MostDetailedMip = 0,
        .MipLevels = 1,
        .FirstArraySlice = 0,
        .ArraySize = g_max_shadow_cascades,
        .PlaneSlice = 0,
        .ResourceMinLODClamp = 0.f 
      }
    };

    LazySRV(srv_desc);
    LazyDSV(dsv_desc);

    Texture2D::loadDerived(res);
  }

  void ShadowTexture::Unload_INTERNAL() { Texture2D::Unload_INTERNAL(); }
}

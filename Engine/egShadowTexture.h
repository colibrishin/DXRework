#pragma once
#include "egTexture2D.h"

namespace Engine::Resources
{
  class ShadowTexture : public Texture2D
  {
  public:
    RESOURCE_T(RES_T_SHADOW_TEX)

    ShadowTexture()
      : Texture2D
      (
       "",
       {
         .Alignment = 0,
         .Width = g_max_shadow_map_size,
         .Height = g_max_shadow_map_size,
         .DepthOrArraySize = g_max_shadow_cascades,
         .Format = DXGI_FORMAT_R32_TYPELESS,
         .Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
         .MipsLevel = 1,
         .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
         .SampleDesc = {1, 0},
       }
      ) { }

    ~ShadowTexture() override = default;

    void          FixedUpdate(const float& dt) override;
    void          Initialize() override;
    void          PostRender(const float& dt) override;
    void          PostUpdate(const float& dt) override;
    void          PreRender(const float& dt) override;
    void          PreUpdate(const float& dt) override;
    void          Render(const float& dt) override;
    void          Update(const float& dt) override;

    void          OnSerialized() override;
    void          OnDeserialized() override;
    void          OnImGui() override;
    eResourceType GetResourceType() const override;

    UINT GetDepth() const override;
    UINT GetHeight() const override;
    UINT GetWidth() const override;

    void Clear(ID3D12GraphicsCommandList1 * cmd) const;

  protected:
    void loadDerived(ComPtr<ID3D12Resource>& res) override;
    void Unload_INTERNAL() override;
    
  };
}
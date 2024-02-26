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
         .Width = g_max_shadow_map_size,
         .Height = g_max_shadow_map_size,
         .Depth = 0,
         .ArraySize = g_max_shadow_cascades,
         .Format = DXGI_FORMAT_R32_TYPELESS,
         .CPUAccessFlags = 0,
         .BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL,
         .MipsLevel = 1,
         .MiscFlags = 0,
         .Usage = D3D11_USAGE_DEFAULT,
         .SampleDesc = {.Count = 1, .Quality = 0}
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

    UINT GetArraySize() const override;
    UINT GetHeight() const override;
    UINT GetWidth() const override;

    void Clear() const;

  protected:
    void loadDerived(ComPtr<ID3D11Resource>& res) override;
    void Unload_INTERNAL() override;
    RESOURCE_SERIALIZER_OVERRIDE(ShadowTexture)

  };
}
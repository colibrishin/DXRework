#pragma once
#include "egD3Device.hpp"
#include "egResource.h"

namespace Engine::Resources
{
  class Texture : public Abstract::Resource
  {
  public:
    RESOURCE_T(RES_T_TEX)

    struct GenericTextureDescription
    {
      UINT        Width = 0;
      UINT        Height = 0;
      UINT        Depth = 0;
      UINT        ArraySize = 0;
      DXGI_FORMAT Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
      UINT        CPUAccessFlags = 0;
      UINT        BindFlags = (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS);
      UINT        MipsLevel = 1;
      UINT        MiscFlags = 0;
      D3D11_USAGE Usage = D3D11_USAGE_DEFAULT;
      DXGI_SAMPLE_DESC SampleDesc = {.Count = 1, .Quality = 0};
    };

    explicit Texture(std::filesystem::path path, const eTexType type, const GenericTextureDescription& description);

    ~Texture() override;

  public:
    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;

    eResourceType GetResourceType() const override;

    eTexBindSlots GetSlot() const;
    UINT          GetSlotOffset() const;
    eShaderType   GetBoundShader() const;
    eTexType      GetPrimitiveTextureType() const;

    ID3D11ShaderResourceView*  GetSRV() const;
    ID3D11RenderTargetView*    GetRTV() const;
    ID3D11DepthStencilView*    GetDSV() const;
    ID3D11UnorderedAccessView* GetUAV() const;

    bool         IsHotload() const;

    void BindAs(const D3D11_BIND_FLAG bind, const eTexBindSlots slot, const UINT slot_offset, const eShaderType shader);
    void Map(const std::function<void(const D3D11_MAPPED_SUBRESOURCE&)> & copy_func) const;

    RESOURCE_SELF_INFER_GETTER(Texture)

  protected:
    Texture();

    // Derived class should hide these by their own case.
    virtual UINT GetWidth() const;
    virtual UINT GetHeight() const;
    virtual UINT GetDepth() const;
    virtual UINT GetArraySize() const;

    void LazyDescription(const GenericTextureDescription & desc);
    void LazyRTV(const D3D11_RENDER_TARGET_VIEW_DESC & desc);
    void LazyDSV(const D3D11_DEPTH_STENCIL_VIEW_DESC & desc);
    void LazyUAV(const D3D11_UNORDERED_ACCESS_VIEW_DESC & desc);
    void LazySRV(const D3D11_SHADER_RESOURCE_VIEW_DESC & desc);

    const GenericTextureDescription& GetDescription() const;

    virtual void loadDerived(ComPtr<ID3D11Resource>& res) = 0;
    void Unload_INTERNAL() override;

    SERIALIZER_ACCESS
    // Non-serialized
    ComPtr<ID3D11Resource>            m_res_;

    ComPtr<ID3D11ShaderResourceView>  m_srv_;
    ComPtr<ID3D11RenderTargetView>    m_rtv_;
    ComPtr<ID3D11DepthStencilView>    m_dsv_;
    ComPtr<ID3D11UnorderedAccessView> m_uav_;

  private:
    void Load_INTERNAL() override final;

    inline static ComPtr<ID3D11RenderTargetView> s_previous_rtv = nullptr;
    inline static ComPtr<ID3D11DepthStencilView> s_previous_dsv = nullptr;

    bool                             m_b_lazy_window_;

    GenericTextureDescription        m_desc_;
    eTexType                         m_type_;

    std::bitset<4>                   m_custom_desc_;
    D3D11_RENDER_TARGET_VIEW_DESC    m_rtv_desc_{};
    D3D11_DEPTH_STENCIL_VIEW_DESC    m_dsv_desc_{};
    D3D11_UNORDERED_ACCESS_VIEW_DESC m_uav_desc_{};
    D3D11_SHADER_RESOURCE_VIEW_DESC  m_srv_desc_{};

    // Non-serialized
    D3D11_BIND_FLAG                  m_bind_to_;
    eTexBindSlots                    m_bound_slot_;
    UINT                             m_bound_slot_offset_;
    eShaderType                      m_bound_shader_;

  };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Texture)

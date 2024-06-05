#pragma once
#include "egD3Device.hpp"
#include "egResource.h"
#include "egResourceManager.hpp"
#include <bitset>

namespace Engine::Resources
{
  class Texture : public Abstract::Resource
  {
  public:
    RESOURCE_T(RES_T_TEX)

    struct GenericTextureDescription
    {
      UINT                 Alignment        = 0;
      UINT                 Width            = 0;
      UINT                 Height           = 0;
      UINT16               DepthOrArraySize = 0;
      DXGI_FORMAT          Format           = DXGI_FORMAT_R32G32B32A32_FLOAT;
      D3D12_RESOURCE_FLAGS Flags            = D3D12_RESOURCE_FLAG_NONE;
      UINT16               MipsLevel        = 1;
      D3D12_TEXTURE_LAYOUT Layout           = D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE;
      DXGI_SAMPLE_DESC     SampleDesc       = {.Count = 1, .Quality = 0};

    private:
      friend class boost::serialization::access;

      template <class Archive>
      void serialize(Archive& ar, const unsigned int version)
      {
        ar & Alignment;
        ar & Width;
        ar & Height;
        ar & DepthOrArraySize;
        ar & Format;
        ar & Flags;
        ar & MipsLevel;
        ar & Layout;
        ar & SampleDesc;
      }
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

    void OnSerialized() override;

    eResourceType GetResourceType() const override;
    eTexType      GetPrimitiveTextureType() const;

    ID3D12DescriptorHeap* GetSRVDescriptor() const;
    ID3D12DescriptorHeap* GetRTVDescriptor() const;
    ID3D12DescriptorHeap* GetDSVDescriptor() const;
    ID3D12DescriptorHeap* GetUAVDescriptor() const;
    ID3D12Resource*       GetRawResoruce() const;

    bool IsHotload() const;

    void Bind(const eCommandList list, const eBindType type, const UINT slot, const UINT offset) const;
    void Bind(const eCommandList list, const Texture& dsv) const;
    
    void Unbind(const eCommandList list, const eBindType type) const;
    void Unbind(const eCommandList list, const Texture& dsv) const;

    RESOURCE_SELF_INFER_GETTER(Texture)

  protected:
    // Derived class should hide these by their own case.
    virtual UINT GetWidth() const;
    virtual UINT GetHeight() const;
    virtual UINT GetDepth() const;

    void LazyDescription(const GenericTextureDescription & desc);
    void LazyRTV(const D3D12_RENDER_TARGET_VIEW_DESC & desc);
    void LazyDSV(const D3D12_DEPTH_STENCIL_VIEW_DESC & desc);
    void LazyUAV(const D3D12_UNORDERED_ACCESS_VIEW_DESC & desc);
    void LazySRV(const D3D12_SHADER_RESOURCE_VIEW_DESC & desc);

    const GenericTextureDescription& GetDescription() const;

    virtual void loadDerived(ComPtr<ID3D12Resource>& res) = 0;
    virtual bool map(char* mapped);

    [[nodiscard]] ID3D12Resource* GetRawResource() const;

    void Unload_INTERNAL() override;

    SERIALIZE_DECL
    // Non-serialized
    ComPtr<ID3D12Resource> m_res_;

    ComPtr<ID3D12DescriptorHeap> m_srv_;
    ComPtr<ID3D12DescriptorHeap> m_rtv_;
    ComPtr<ID3D12DescriptorHeap> m_dsv_;
    ComPtr<ID3D12DescriptorHeap> m_uav_;

  private:
    Texture();
    void Load_INTERNAL() override final;

    void InitializeDescriptorHeaps();

    void mapInternal();

    GenericTextureDescription        m_desc_;
    eTexType                         m_type_;

    bool                             m_custom_desc_[4];
    D3D12_RENDER_TARGET_VIEW_DESC    m_rtv_desc_{};
    D3D12_DEPTH_STENCIL_VIEW_DESC    m_dsv_desc_{};
    D3D12_UNORDERED_ACCESS_VIEW_DESC m_uav_desc_{};
    D3D12_SHADER_RESOURCE_VIEW_DESC  m_srv_desc_{};

    // Non-serialized
    bool                             m_b_lazy_window_;
    ComPtr<ID3D12Resource>           m_upload_buffer_;

    RTVDSVHandlePair                 m_previous_handles_;

  };
} // namespace Engine::Resources

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Resources::Texture)
BOOST_CLASS_EXPORT_KEY(Engine::Resources::Texture)

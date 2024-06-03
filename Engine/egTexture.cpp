#include "pch.h"
#include <DirectXTex.h>
#include <d3dx12.h>
#include <wincodec.h>

#include "egTexture.h"
#include "egD3Device.hpp"
#include "egRenderPipeline.h"

SERIALIZE_IMPL
(
    Engine::Resources::Texture, 
    _ARTAG(_BSTSUPER(Resource))
    _ARTAG(m_desc_) _ARTAG(m_type_)
    _ARTAG(m_custom_desc_) _ARTAG(m_rtv_desc_)
    _ARTAG(m_dsv_desc_) _ARTAG(m_uav_desc_)
    _ARTAG(m_srv_desc_)
)

namespace Engine::Resources
{
  Texture::Texture(std::filesystem::path path, const eTexType type, const GenericTextureDescription& description)
    : Resource(std::move(path), RES_T_TEX),
      m_desc_(description),
      m_type_(type),
      m_custom_desc_{false},
      m_b_lazy_window_(true),
      m_bound_type_(BIND_TYPE_SRV),
      m_bound_slot_(BIND_SLOT_TEX),
      m_bound_slot_offset_(0) {}

  eTexBindSlots Texture::GetSlot() const { return m_bound_slot_; }

  UINT Texture::GetSlotOffset() const { return m_bound_slot_offset_; }

  eTexType Texture::GetPrimitiveTextureType() const { return m_type_; }

  ID3D12DescriptorHeap* Texture::GetSRVDescriptor() const { return m_srv_.Get(); }

  ID3D12DescriptorHeap* Texture::GetRTVDescriptor() const { return m_rtv_.Get(); }

  ID3D12DescriptorHeap* Texture::GetDSVDescriptor() const { return m_dsv_.Get(); }

  ID3D12DescriptorHeap* Texture::GetUAVDescriptor() const { return m_uav_.Get(); }

  ID3D12Resource* Texture::GetRawResoruce() const
  {
    return m_res_.Get();
  }

  bool Texture::IsHotload() const { return GetPath().empty(); }

  void Texture::BindAs(const eBindType type, const eTexBindSlots slot, const UINT slot_offset)
  {
    if (type == BIND_TYPE_SAMPLER || type == BIND_TYPE_CB)
    {
      throw std::runtime_error("Cannot bind texture as sampler or constant buffer");
    }

    m_bound_type_   = type;
    m_bound_slot_   = slot;
    m_bound_slot_offset_ = slot_offset;
  }

  void Texture::mapInternal()
  {
    const auto& total_size = m_desc_.Width * m_desc_.Height * m_desc_.DepthOrArraySize;

    const auto& dest_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_COPY_DEST
      );

    DX::ThrowIfFailed
    (
        DirectX::CreateUploadBuffer
        (
            GetD3Device().GetDevice(),
            nullptr,
            total_size,
            DirectX::BitsPerPixel(m_desc_.Format) / 8,
            m_upload_buffer_.GetAddressOf()
        )
    );

    char* mapped = nullptr;

    DX::ThrowIfFailed(
        m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&mapped)));

    const bool map_flag = map(mapped);

    m_upload_buffer_->Unmap(0, nullptr);

    if (!map_flag)
    {
      return;
    }

    GetD3Device().WaitAndReset(COMMAND_IDX_COPY);

    GetD3Device().GetCopyCommandList()->ResourceBarrier(1, &dest_transition);

    auto dst = CD3DX12_TEXTURE_COPY_LOCATION(m_res_.Get(), 0);
    auto src = CD3DX12_TEXTURE_COPY_LOCATION(m_upload_buffer_.Get(), 0);
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = {0, {m_desc_.Format, m_desc_.Width, m_desc_.Height, m_desc_.DepthOrArraySize}};
    src.PlacedFootprint.Footprint.RowPitch = m_desc_.Width * DirectX::BitsPerPixel(m_desc_.Format) / 8;
    src.PlacedFootprint.Footprint.Depth = m_desc_.DepthOrArraySize;
    src.PlacedFootprint.Footprint.Width = m_desc_.Width;
    src.PlacedFootprint.Footprint.Height = m_desc_.Height;
    src.PlacedFootprint.Footprint.Format = m_desc_.Format;

    GetD3Device().GetCopyCommandList()->CopyTextureRegion
      (
       &dst,
       0, 0, 0,
       &src,
       nullptr
      );

    GetD3Device().ExecuteCopyCommandList();
  }

  Texture::Texture()
    : Resource("", RES_T_TEX),
      m_desc_({}),
      m_type_(TEX_TYPE_2D),
      m_custom_desc_{false},
      m_b_lazy_window_(true),
      m_bound_type_(BIND_TYPE_SRV),
      m_bound_slot_(BIND_SLOT_TEX),
      m_bound_slot_offset_(0) {}

  UINT Texture::GetWidth() const { return m_desc_.Width; }

  UINT Texture::GetHeight() const { return m_desc_.Height; }

  UINT Texture::GetDepth() const { return m_desc_.DepthOrArraySize; }

  void Texture::LazyDescription(const GenericTextureDescription& desc)
  {
    if (!m_b_lazy_window_)
    {
      throw std::runtime_error("Texture is already created with given description, Cannot initialize lazily");
    }

    m_desc_ = desc;
  }

  void Texture::LazyRTV(const   D3D12_RENDER_TARGET_VIEW_DESC& desc)
  {
    if (!m_b_lazy_window_)
    {
      throw std::runtime_error("Texture is already created with given description, Cannot initialize lazily");
    }

    m_custom_desc_[0] = true;
    m_rtv_desc_ = desc;
  }

  void Texture::LazyDSV(const  D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
  {
    if (!m_b_lazy_window_)
    {
      throw std::runtime_error("Texture is already created with given description, Cannot initialize lazily");
    }

    m_custom_desc_[1] = true;
    m_dsv_desc_ = desc;
  }

  void Texture::LazyUAV(const  D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
  {
    if (!m_b_lazy_window_)
    {
      throw std::runtime_error("Texture is already created with given description, Cannot initialize lazily");
    }

    m_custom_desc_[2] = true;
    m_uav_desc_ = desc;
  }

  void Texture::LazySRV(const  D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
  {
    if (!m_b_lazy_window_)
    {
      throw std::runtime_error("Texture is already created with given description, Cannot initialize lazily");
    }

    m_custom_desc_[3] = true;
    m_srv_desc_ = desc;
  }

  const Texture::GenericTextureDescription& Texture::GetDescription() const { return m_desc_; }

  Texture::~Texture() = default;

  void Texture::Initialize() { }

  void Texture::PreUpdate(const float& dt) {}

  void Texture::Update(const float& dt) {}

  void Texture::PreRender(const float& dt) {}

  void Texture::Render(const float& dt)
  {
    if (m_bound_type_ == BIND_TYPE_RTV)
    {
      const auto& rtv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_RENDER_TARGET
      );

      GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &rtv_transition);

      m_previous_handles_ = GetRenderPipeline().SetRenderTargetDeferred(
      m_rtv_->GetCPUDescriptorHandleForHeapStart());
    }
    else if (m_bound_type_ == BIND_TYPE_SRV)
    {
      const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
      );

      GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &srv_transition);

      GetRenderPipeline().SetShaderResource(
          m_srv_->GetCPUDescriptorHandleForHeapStart(), 
          m_bound_slot_ + m_bound_slot_offset_);
    }
    else if (m_bound_type_ == BIND_TYPE_DSV)
    {
      const auto& dsv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_DEPTH_WRITE
      );

      GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &dsv_transition);

      m_previous_handles_ = GetRenderPipeline().SetDepthStencilDeferred(
          m_dsv_->GetCPUDescriptorHandleForHeapStart());
    }
    else if (m_bound_type_ == BIND_TYPE_DSV_ONLY)
    {
      const auto& dsv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_DEPTH_WRITE
      );

      GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &dsv_transition);

      m_previous_handles_ = GetRenderPipeline().SetDepthStencilOnlyDeferred(
          m_dsv_->GetCPUDescriptorHandleForHeapStart());
    }
    else if (m_bound_type_ == BIND_TYPE_UAV)
    {
      const auto& uav_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_UNORDERED_ACCESS
      );

      GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &uav_transition);

      GetRenderPipeline().SetUnorderedAccess(
          m_uav_->GetCPUDescriptorHandleForHeapStart(), 
          m_bound_slot_ + m_bound_slot_offset_);
    }
  }

  void Texture::PostRender(const float& dt)
  {
    const auto& rtv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_RENDER_TARGET,
       D3D12_RESOURCE_STATE_COMMON
      );

    const auto& dsv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_DEPTH_WRITE,
       D3D12_RESOURCE_STATE_COMMON
      );

    const auto& uav_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
       D3D12_RESOURCE_STATE_COMMON
      );

    const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
       D3D12_RESOURCE_STATE_COMMON
      );

    switch (m_bound_type_)
    {
      case BIND_TYPE_RTV:
        GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &rtv_transition);

        GetRenderPipeline().SetRenderTargetDeferred(m_previous_handles_);
        break;
      case BIND_TYPE_DSV:
      case BIND_TYPE_DSV_ONLY:
          GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &dsv_transition);

          GetRenderPipeline().SetRenderTargetDeferred(m_previous_handles_);
          break;
      case BIND_TYPE_UAV: 
          GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &uav_transition);
        break;
      case BIND_TYPE_SRV: 
          GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &srv_transition);
          break;
      default:
        throw std::runtime_error("Unknown bind type");
        break;
    }
  }

  void Texture::PostUpdate(const float& dt) {}

  void Texture::InitializeDescriptorHeaps()
  {
    constexpr D3D12_DESCRIPTOR_HEAP_DESC buffer_desc = 
    {
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      1,
      D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      0
    };

    constexpr D3D12_DESCRIPTOR_HEAP_DESC rtv_desc = 
    {
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
      1,
      D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      0
    };

    constexpr D3D12_DESCRIPTOR_HEAP_DESC dsv_desc = 
    {
      D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
      1,
      D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      0
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &buffer_desc,
        IID_PPV_ARGS(m_srv_.ReleaseAndGetAddressOf())
       )
      );

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &rtv_desc,
        IID_PPV_ARGS(m_rtv_.ReleaseAndGetAddressOf())
       )
      );

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &dsv_desc,
        IID_PPV_ARGS(m_dsv_.ReleaseAndGetAddressOf())
       )
      );

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &buffer_desc,
        IID_PPV_ARGS(m_uav_.ReleaseAndGetAddressOf())
       )
      );
  }

  void Texture::Load_INTERNAL()
  {
    if ((m_desc_.Flags & D3D11_BIND_DEPTH_STENCIL) && (m_desc_.Flags & D3D11_BIND_UNORDERED_ACCESS))
    {
      throw std::logic_error("Depth stencil and unordered cannot be flagged in same texture");
    }

    loadDerived(m_res_);

    if (!GetPath().empty())
    {
      GetD3Device().CreateTextureFromFile
        (
         absolute(GetPath()),
         m_res_.ReleaseAndGetAddressOf(),
         false
        );

      const D3D12_RESOURCE_DESC desc = m_res_->GetDesc();

      if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
      {
        m_type_ = TEX_TYPE_1D;
      }
      else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
      {
        m_type_ = TEX_TYPE_2D;
      }
      else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
      {
        m_type_ = TEX_TYPE_3D;
      }
      else
      {
        throw std::runtime_error("Unknown type is loaded into texture");
      }

      m_desc_.Alignment = desc.Alignment;
      m_desc_.MipsLevel = desc.MipLevels;
      m_desc_.SampleDesc = desc.SampleDesc;
      m_desc_.Layout = desc.Layout;
      m_desc_.Flags = desc.Flags;
      m_desc_.Format = desc.Format;
      m_desc_.DepthOrArraySize = desc.DepthOrArraySize;
      m_desc_.Width = desc.Width;
      m_desc_.Height = desc.Height;
    }
    else
    {
      const auto& heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

      D3D12_RESOURCE_DIMENSION dim;

      switch (m_type_)
      {
        case TEX_TYPE_1D:
          dim = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
          break;
        case TEX_TYPE_2D:
          dim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
          break;
        case TEX_TYPE_3D:
          dim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
          break;
        default:
          throw std::runtime_error("Unknown texture type");
      }

      const D3D12_RESOURCE_DESC desc
      {
        .Dimension = dim,
        .Alignment = m_desc_.Alignment,
        .Width = m_desc_.Width,
        .Height = m_desc_.Height,
        .DepthOrArraySize = m_desc_.DepthOrArraySize,
        .MipLevels = m_desc_.MipsLevel,
        .Format = m_desc_.Format,
        .SampleDesc = m_desc_.SampleDesc,
        .Layout = m_desc_.Layout,
        .Flags = m_desc_.Flags
      };

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &heap_prop,
          D3D12_HEAP_FLAG_NONE,
          &desc,
          D3D12_RESOURCE_STATE_COMMON,
          nullptr,
          IID_PPV_ARGS(m_res_.GetAddressOf())
         )
        );
    }

    mapInternal();

    InitializeDescriptorHeaps();

    if (m_custom_desc_[3])
    {
      GetD3Device().GetDevice()->CreateShaderResourceView(
          m_res_.Get(), 
          &m_srv_desc_, 
          m_srv_->GetCPUDescriptorHandleForHeapStart());
    }
    else
    {
      GetD3Device().GetDevice()->CreateShaderResourceView(
          m_res_.Get(),
          nullptr,
          m_srv_->GetCPUDescriptorHandleForHeapStart());
    }

    if (m_custom_desc_[0] && m_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
      GetD3Device().GetDevice()->CreateRenderTargetView
        (m_res_.Get(), &m_rtv_desc_, m_srv_->GetCPUDescriptorHandleForHeapStart());
    }
    else if (m_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
      GetD3Device().GetDevice()->CreateRenderTargetView
        (m_res_.Get(), nullptr, m_rtv_->GetCPUDescriptorHandleForHeapStart());
    }

    if (m_custom_desc_[1] && m_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
      GetD3Device().GetDevice()->CreateDepthStencilView(m_res_.Get(), &m_dsv_desc_, m_dsv_->GetCPUDescriptorHandleForHeapStart());
    }
    else if (m_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
      GetD3Device().GetDevice()->CreateDepthStencilView(m_res_.Get(), nullptr, m_dsv_->GetCPUDescriptorHandleForHeapStart());
    }

    if (m_custom_desc_[2] && m_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    {
      GetD3Device().GetDevice()->CreateUnorderedAccessView(m_res_.Get(), nullptr, &m_uav_desc_, m_uav_->GetCPUDescriptorHandleForHeapStart());
    }
    else if (m_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    {
      GetD3Device().GetDevice()->CreateUnorderedAccessView(m_res_.Get(), nullptr, nullptr, m_uav_->GetCPUDescriptorHandleForHeapStart());
    }

    m_b_lazy_window_ = false;
  }

  bool Texture::map(char* mapped)
  {
    return false;
  }

  void Texture::Unload_INTERNAL()
  {
    m_srv_.Reset();
    m_rtv_.Reset();
    m_dsv_.Reset();
    m_uav_.Reset();
    m_res_.Reset();
    m_b_lazy_window_ = true;
  }

  void Texture::FixedUpdate(const float& dt) {}

  void Texture::OnSerialized()
  {
    const auto name = GetName();
    const std::filesystem::path folder = GetPrettyTypeName();
    const std::filesystem::path filename = name + ".dds";
    const std::filesystem::path final_path = folder / filename;

    if (!IsLoaded())
    {
      Load();
    }

    if (m_res_)
    {
      DirectX::ScratchImage image;

      DX::ThrowIfFailed
        (
         DirectX::CaptureTexture
         (
          GetD3Device().GetCopyCommandQueue(), m_res_.Get(), false, image, D3D12_RESOURCE_STATE_COMMON,
          D3D12_RESOURCE_STATE_COMMON
         )
        );

      if (!std::filesystem::exists(folder))
      {
        std::filesystem::create_directories(folder);
      }

      DX::ThrowIfFailed
        (
         DirectX::SaveToDDSFile
         (
             image.GetImages(),
             image.GetImageCount(),
             image.GetMetadata(),
             DirectX::DDS_FLAGS_ALLOW_LARGE_FILES,
             final_path.c_str()
         )
        );

      SetPath(final_path);
    }
  }

  eResourceType Texture::GetResourceType() const { return Resource::GetResourceType(); }
} // namespace Engine::Resources

#include "pch.h"
#include <DirectXTex.h>
#include <d3dx12.h>
#include <wincodec.h>

#include "egTexture.h"

#include "egCommands.h"
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

  eTexType Texture::GetPrimitiveTextureType() const { return m_type_; }

  ID3D12DescriptorHeap* Texture::GetSRVDescriptor() const { return m_srv_.Get(); }

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

  void Texture::Map(const std::function<void(char*)>& copy_func) const
  {
    char* mapped = nullptr;

    DX::ThrowIfFailed(
        m_res_->Map(0, nullptr, reinterpret_cast<void**>(&mapped)));

    copy_func(mapped);

    m_res_->Unmap(0, nullptr);
  }

  void Texture::Bind(const CommandPair& cmd, const Texture& dsv) const
  {
    const auto& rtv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_RENDER_TARGET
      );

    const auto& dsv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       dsv.GetRawResource(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_DEPTH_WRITE
      );

    cmd.GetList()->ResourceBarrier(1, &rtv_trans);

    cmd.GetList()->ResourceBarrier(1, &dsv_trans);

    const D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle[]
    {
      m_rtv_->GetCPUDescriptorHandleForHeapStart()
    };

    const D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle[]
    {
      dsv.GetDSVDescriptor()->GetCPUDescriptorHandleForHeapStart()
    };

    cmd.GetList()->OMSetRenderTargets
      (
       1,
       rtv_handle,
       false,
       dsv_handle
      );
  }

  void Texture::Unbind(const CommandPair& cmd, const eBindType type) const
  {
    Unbind(cmd.GetList(), type);
  }

  void Texture::Unbind(ID3D12GraphicsCommandList1* cmd, const eBindType type) const
  {
    const auto& srv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
       D3D12_RESOURCE_STATE_COMMON
      );

    const auto& uav_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
       D3D12_RESOURCE_STATE_COMMON
      );

    const auto& rtv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_RENDER_TARGET,
       D3D12_RESOURCE_STATE_COMMON
      );

    const auto& dsv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_DEPTH_WRITE,
       D3D12_RESOURCE_STATE_COMMON
      );

    switch (type)
    {
    case BIND_TYPE_UAV: 
        cmd->ResourceBarrier(1, &uav_trans);
        break;
    case BIND_TYPE_SRV:
        cmd->ResourceBarrier(1, &srv_trans);
        break;
    case BIND_TYPE_RTV:
        cmd->ResourceBarrier(1, &rtv_trans);
        break;
    case BIND_TYPE_DSV:
    case BIND_TYPE_DSV_ONLY:
        cmd->ResourceBarrier(1, &dsv_trans);
      break;
    case BIND_TYPE_SAMPLER: break;
    case BIND_TYPE_CB: break;
    case BIND_TYPE_COUNT: break;
    default: ;
    }
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

    const auto& dst_transition_back = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COPY_DEST,
       D3D12_RESOURCE_STATE_COMMON
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

    const auto& cmd = GetD3Device().GetCommandList(COMMAND_LIST_COPY);

    GetD3Device().WaitAndReset(COMMAND_LIST_COPY);

    cmd->ResourceBarrier(1, &dest_transition);

    const auto dst = CD3DX12_TEXTURE_COPY_LOCATION(m_res_.Get(), 0);
    auto src = CD3DX12_TEXTURE_COPY_LOCATION(m_upload_buffer_.Get(), 0);
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = {0, {m_desc_.Format, m_desc_.Width, m_desc_.Height, m_desc_.DepthOrArraySize}};
    src.PlacedFootprint.Footprint.RowPitch = m_desc_.Width * DirectX::BitsPerPixel(m_desc_.Format) / 8;
    src.PlacedFootprint.Footprint.Depth = m_desc_.DepthOrArraySize;
    src.PlacedFootprint.Footprint.Width = m_desc_.Width;
    src.PlacedFootprint.Footprint.Height = m_desc_.Height;
    src.PlacedFootprint.Footprint.Format = m_desc_.Format;

    cmd->CopyTextureRegion
      (
       &dst,
       0, 0, 0,
       &src,
       nullptr
      );

    cmd->ResourceBarrier(1, &dst_transition_back);

    GetD3Device().ExecuteCommandList(COMMAND_LIST_COPY);

    GetD3Device().Wait(COMMAND_TYPE_COPY);
  }

  void Texture::Bind(const CommandPair& cmd, const DescriptorPtr& heap, const Texture& dsv) const
  {
    const auto& rtv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_RENDER_TARGET
      );

    const auto& dsv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       dsv.GetRawResource(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_DEPTH_WRITE
      );

    cmd.GetList()->ResourceBarrier(1, &rtv_trans);

    cmd.GetList()->ResourceBarrier(1, &dsv_trans);

    const D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle[]
    {
      m_rtv_->GetCPUDescriptorHandleForHeapStart()
    };

    const D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle[]
    {
      dsv.GetDSVDescriptor()->GetCPUDescriptorHandleForHeapStart()
    };

    cmd.GetList()->OMSetRenderTargets
      (
       1,
       rtv_handle,
       false,
       dsv_handle
      );
  }

  void Texture::Unbind(const CommandPair& cmd, const eBindType type) const
  {
    Unbind(cmd.GetList(), type);
  }

  void Texture::Unbind(ID3D12GraphicsCommandList1* cmd, const eBindType type) const
  {
    const auto& srv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
       D3D12_RESOURCE_STATE_COMMON
      );

    const auto& uav_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
       D3D12_RESOURCE_STATE_COMMON
      );

    const auto& rtv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_RENDER_TARGET,
       D3D12_RESOURCE_STATE_COMMON
      );

    const auto& dsv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_DEPTH_WRITE,
       D3D12_RESOURCE_STATE_COMMON
      );

    switch (type)
    {
    case BIND_TYPE_UAV: 
        cmd->ResourceBarrier(1, &uav_trans);
        break;
    case BIND_TYPE_SRV:
        cmd->ResourceBarrier(1, &srv_trans);
        break;
    case BIND_TYPE_RTV:
        cmd->ResourceBarrier(1, &rtv_trans);
        break;
    case BIND_TYPE_DSV:
    case BIND_TYPE_DSV_ONLY:
        cmd->ResourceBarrier(1, &dsv_trans);
      break;
    case BIND_TYPE_SAMPLER: break;
    case BIND_TYPE_CB: break;
    case BIND_TYPE_COUNT: break;
    default: ;
    }
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

    const auto& dst_transition_back = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COPY_DEST,
       D3D12_RESOURCE_STATE_COMMON
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

    const auto& cmd = GetD3Device().GetCommandList(COMMAND_LIST_COPY);

    GetD3Device().WaitAndReset(COMMAND_LIST_COPY);

    cmd->ResourceBarrier(1, &dest_transition);

    const auto dst = CD3DX12_TEXTURE_COPY_LOCATION(m_res_.Get(), 0);
    auto src = CD3DX12_TEXTURE_COPY_LOCATION(m_upload_buffer_.Get(), 0);
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = {0, {m_desc_.Format, m_desc_.Width, m_desc_.Height, m_desc_.DepthOrArraySize}};
    src.PlacedFootprint.Footprint.RowPitch = m_desc_.Width * DirectX::BitsPerPixel(m_desc_.Format) / 8;
    src.PlacedFootprint.Footprint.Depth = m_desc_.DepthOrArraySize;
    src.PlacedFootprint.Footprint.Width = m_desc_.Width;
    src.PlacedFootprint.Footprint.Height = m_desc_.Height;
    src.PlacedFootprint.Footprint.Format = m_desc_.Format;

    cmd->CopyTextureRegion
      (
       &dst,
       0, 0, 0,
       &src,
       nullptr
      );

    cmd->ResourceBarrier(1, &dst_transition_back);

    GetD3Device().ExecuteCommandList(COMMAND_LIST_COPY);

    GetD3Device().Wait(COMMAND_TYPE_COPY);
  }

  void Texture::Bind(const eCommandList list, const Texture& dsv) const
  {
    const auto& rtv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_RENDER_TARGET
      );

    const auto& dsv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       dsv.GetRawResource(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_DEPTH_WRITE
      );

    GetD3Device().GetCommandList(list)->ResourceBarrier(1, &rtv_trans);

    GetD3Device().GetCommandList(list)->ResourceBarrier(1, &dsv_trans);

    GetRenderPipeline().SetRenderTargetDeferred
    (
        list, 
        m_rtv_->GetCPUDescriptorHandleForHeapStart(),
        dsv.GetDSVDescriptor()->GetCPUDescriptorHandleForHeapStart()
    );
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

    const auto& dst_transition_back = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COPY_DEST,
       D3D12_RESOURCE_STATE_COMMON
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

    GetD3Device().WaitAndReset(COMMAND_LIST_COPY);

    GetD3Device().GetCommandList(COMMAND_LIST_COPY)->ResourceBarrier(1, &dest_transition);

    auto dst = CD3DX12_TEXTURE_COPY_LOCATION(m_res_.Get(), 0);
    auto src = CD3DX12_TEXTURE_COPY_LOCATION(m_upload_buffer_.Get(), 0);
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = {0, {m_desc_.Format, m_desc_.Width, m_desc_.Height, m_desc_.DepthOrArraySize}};
    src.PlacedFootprint.Footprint.RowPitch = m_desc_.Width * DirectX::BitsPerPixel(m_desc_.Format) / 8;
    src.PlacedFootprint.Footprint.Depth = m_desc_.DepthOrArraySize;
    src.PlacedFootprint.Footprint.Width = m_desc_.Width;
    src.PlacedFootprint.Footprint.Height = m_desc_.Height;
    src.PlacedFootprint.Footprint.Format = m_desc_.Format;

    GetD3Device().GetCommandList(COMMAND_LIST_COPY)->CopyTextureRegion
      (
       &dst,
       0, 0, 0,
       &src,
       nullptr
      );

    GetD3Device().GetCommandList(COMMAND_LIST_COPY)->ResourceBarrier(1, &dst_transition_back);

    GetD3Device().ExecuteCommandList(COMMAND_LIST_COPY);
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
      m_previous_handles_ = GetRenderPipeline().SetRenderTargetDeferred(
      m_rtv_->GetCPUDescriptorHandleForHeapStart());
    }
    else if (m_bound_type_ == BIND_TYPE_SRV)
    {
      GetRenderPipeline().SetShaderResource(
          m_srv_->GetCPUDescriptorHandleForHeapStart(), 
          m_bound_slot_ + m_bound_slot_offset_);
    }
    else if (m_bound_type_ == BIND_TYPE_DSV)
    {
      m_previous_handles_ = GetRenderPipeline().SetDepthStencilOnlyDeferred(
          m_dsv_->GetCPUDescriptorHandleForHeapStart());
    }
    else if (m_bound_type_ == BIND_TYPE_UAV)
    {
      GetRenderPipeline().SetUnorderedAccess(
          m_uav_->GetCPUDescriptorHandleForHeapStart(), 
          m_bound_slot_ + m_bound_slot_offset_);
    }
  }

  void Texture::PostUpdate(const float& dt) {}

  void Texture::InitializeDescriptorHeaps()
  {
    switch (m_bound_type_)
    {
      case BIND_TYPE_RTV:
      case BIND_TYPE_DSV:
        GetRenderPipeline().SetRenderTargetDeferred(m_previous_handles_);
        break;
      case BIND_TYPE_UAV:
      case BIND_TYPE_SRV:
      default:
        // todo: unbind or just keep it?
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
      const UINT flag = m_desc_.Flags;

      GetD3Device().CreateTextureFromFile
        (
         absolute(GetPath()),
         m_res_.ReleaseAndGetAddressOf(),
         false
        );

      GetD3Device().WaitAndReset(COMMAND_LIST_UPDATE);

      const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_res_.Get(),
         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
         D3D12_RESOURCE_STATE_COMMON
        );

      GetD3Device().GetCommandList(COMMAND_LIST_UPDATE)->ResourceBarrier(1, &common_transition);

      GetD3Device().ExecuteCommandList(COMMAND_LIST_UPDATE);

      GetD3Device().Wait();

      const D3D12_RESOURCE_DESC desc = m_res_->GetDesc();

      if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
      {
        m_type_ = TEX_TYPE_1D;
        m_desc_.Format = desc.Format;
        m_desc_.DepthOrArraySize = desc.ArraySize;
        m_desc_.Width = desc.Width;
        m_desc_.Height = 0;
      }
      else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
      {
        m_type_ = TEX_TYPE_2D;
        m_desc_.Format = desc.Format;
        m_desc_.DepthOrArraySize = desc.ArraySize;
        m_desc_.Width = desc.Width;
        m_desc_.Height = desc.Height;
      }
      else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
      {
        m_type_ = TEX_TYPE_3D;
        m_desc_.Format = desc.Format;
        m_desc_.DepthOrArraySize = desc.Depth;
        m_desc_.Width = desc.Width;
        m_desc_.Height = desc.Height;
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
      loadDerived(m_res_);

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

    InitializeDescriptorHeaps();

    // todo: lazy initialization is not necessary now.
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

    if (m_custom_desc_[0])
    {
      GetD3Device().GetDevice()->CreateRenderTargetView
        (m_res_.Get(), &m_rtv_desc_, m_srv_->GetCPUDescriptorHandleForHeapStart());
    }
    else
    {
      GetD3Device().GetDevice()->CreateRenderTargetView
        (m_res_.Get(), nullptr, m_rtv_->GetCPUDescriptorHandleForHeapStart());
    }

    if (m_custom_desc_[1])
    {
      GetD3Device().GetDevice()->CreateDepthStencilView(m_res_.Get(), &m_dsv_desc_, m_dsv_->GetCPUDescriptorHandleForHeapStart());
    }
    else
    {
      GetD3Device().GetDevice()->CreateDepthStencilView(m_res_.Get(), nullptr, m_dsv_->GetCPUDescriptorHandleForHeapStart());
    }

    if (m_custom_desc_[2])
    {
      GetD3Device().GetDevice()->CreateUnorderedAccessView(m_res_.Get(), nullptr, &m_uav_desc_, m_uav_->GetCPUDescriptorHandleForHeapStart());
    }
    else
    {
      GetD3Device().GetDevice()->CreateUnorderedAccessView(m_res_.Get(), nullptr, nullptr, m_uav_->GetCPUDescriptorHandleForHeapStart());
    }

    m_b_lazy_window_ = false;
  }

  bool Texture::map(char* mapped)
  {
    return false;
  }

  ID3D12Resource* Texture::GetRawResource() const
  {
    return m_res_.Get();
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
             GetD3Device().GetCommandQueue(), m_res_.Get(), false, image /* + state transition */
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

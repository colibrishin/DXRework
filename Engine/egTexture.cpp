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
      m_b_lazy_window_(true) {}

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

  void Texture::Unbind(const eCommandList list, const Texture& dsv) const
  {
    const auto& rtv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_RENDER_TARGET,
       D3D12_RESOURCE_STATE_COMMON
      );

    const auto& dsv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       dsv.GetRawResource(),
       D3D12_RESOURCE_STATE_DEPTH_WRITE,
       D3D12_RESOURCE_STATE_COMMON
      );

    GetD3Device().GetCommandList(list)->ResourceBarrier(1, &rtv_transition);
    GetD3Device().GetCommandList(list)->ResourceBarrier(1, &dsv_transition);
  }

  void Texture::Unbind(const Weak<CommandPair>& w_cmd, const Texture& dsv) const
  {
    const auto& cmd = w_cmd.lock();

    const auto& rtv_transition_back = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_RENDER_TARGET,
       D3D12_RESOURCE_STATE_COMMON
      );

    const auto& dsv_transition_back = CD3DX12_RESOURCE_BARRIER::Transition
      (
       dsv.GetRawResource(),
       D3D12_RESOURCE_STATE_DEPTH_WRITE,
       D3D12_RESOURCE_STATE_COMMON
      );

    cmd->GetList()->ResourceBarrier(1, &rtv_transition_back);
    cmd->GetList()->ResourceBarrier(1, &dsv_transition_back);
  }

  void Texture::Unbind(const Weak<CommandPair>& w_cmd, Texture** rtvs, const UINT count, const Texture& dsv)
  {
    const auto& cmd = w_cmd.lock();

    std::vector<D3D12_RESOURCE_BARRIER> transitions;
    transitions.reserve(count + 1);

    for (int i = 0; i < count; ++i)
    {
      const auto& rtv_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         rtvs[i]->GetRawResource(),
         D3D12_RESOURCE_STATE_RENDER_TARGET,
         D3D12_RESOURCE_STATE_COMMON
        );

      transitions.push_back(rtv_transition);
    }

    const auto& dsv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       dsv.GetRawResource(),
       D3D12_RESOURCE_STATE_DEPTH_WRITE,
       D3D12_RESOURCE_STATE_COMMON
      );

    transitions.push_back(dsv_transition);

    cmd->GetList()->ResourceBarrier(transitions.size(), transitions.data());
  }

  void Texture::ManualTransition(
    ID3D12GraphicsCommandList1* cmd, const D3D12_RESOURCE_STATES before, const D3D12_RESOURCE_STATES after
  ) const
  {
    const auto& transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       before,
       after
      );

    cmd->ResourceBarrier(1, &transition);
  }

  void Texture::Clear(ID3D12GraphicsCommandList1* cmd, const D3D12_RESOURCE_STATES as) const
  {
    if (as == D3D12_RESOURCE_STATE_RENDER_TARGET)
    {
      constexpr float clear_color[4] = {0.f, 0.f, 0.f, 1.f};

      const auto& transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         GetRawResoruce(),
         D3D12_RESOURCE_STATE_COMMON,
         D3D12_RESOURCE_STATE_RENDER_TARGET
        );

      const auto& transition_back = CD3DX12_RESOURCE_BARRIER::Transition
        (
         GetRawResoruce(),
         D3D12_RESOURCE_STATE_RENDER_TARGET,
         D3D12_RESOURCE_STATE_COMMON
        );

      cmd->ResourceBarrier(1, &transition);

      cmd->ClearRenderTargetView
        (
         GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
         clear_color,
         0,
         nullptr
        );

      cmd->ResourceBarrier(1, &transition_back);
    }
    else if (as == D3D12_RESOURCE_STATE_DEPTH_WRITE)
    {
      const auto& transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         GetRawResoruce(),
         D3D12_RESOURCE_STATE_COMMON,
         D3D12_RESOURCE_STATE_DEPTH_WRITE
        );

      const auto& transition_back = CD3DX12_RESOURCE_BARRIER::Transition
        (
         GetRawResoruce(),
         D3D12_RESOURCE_STATE_DEPTH_WRITE,
         D3D12_RESOURCE_STATE_COMMON
        );

      cmd->ResourceBarrier(1, &transition);

      cmd->ClearDepthStencilView
        (
         GetDSVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
         D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
         1.f,
         0,
         0,
         nullptr
        );

      cmd->ResourceBarrier(1, &transition_back);
    }
  }

  void Texture::Bind(const Weak<CommandPair>& w_cmd, const DescriptorPtr& w_heap, const eBindType type, const UINT slot, const UINT offset) const
  {
    Bind(w_cmd.lock()->GetList(), w_heap, type, slot, offset);
  }

  void Texture::Bind(
    ID3D12GraphicsCommandList1* cmd, const DescriptorPtr& w_heap, const eBindType type, const UINT slot,
    const UINT offset
  ) const
  {
    const auto& heap = w_heap.lock();

    const auto& rtv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_RENDER_TARGET
      );

    const auto& dsv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_DEPTH_WRITE
      );

    const auto& srv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
      );

    const auto& uav_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_res_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_UNORDERED_ACCESS
      );

    switch (type)
    {
    case BIND_TYPE_SRV: 
      {
        cmd->ResourceBarrier(1, &srv_trans);
        heap->SetShaderResource(m_srv_->GetCPUDescriptorHandleForHeapStart(), slot + offset);
        break;
      }
    case BIND_TYPE_UAV:
      {
        cmd->ResourceBarrier(1, &uav_trans);
        heap->SetUnorderedAccess(m_uav_->GetCPUDescriptorHandleForHeapStart(), slot + offset);
        break;
      }
    case BIND_TYPE_RTV:
      {
        cmd->ResourceBarrier(1, &rtv_trans);

        const D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle[]
        {
          m_rtv_->GetCPUDescriptorHandleForHeapStart()
        };

        cmd->OMSetRenderTargets
        (
            1, 
            rtv_handle,
            false,
            nullptr
          );
        break;
      }
    case BIND_TYPE_DSV:
      {
        
        break;
      }
    case BIND_TYPE_DSV_ONLY:
      {
        cmd->ResourceBarrier(1, &dsv_trans);

        const D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle[]
        {
          m_dsv_->GetCPUDescriptorHandleForHeapStart()
        };

        cmd->OMSetRenderTargets
          (
           0,
           nullptr,
           false,
           dsv_handle
          );

        break;
      }
    case BIND_TYPE_SAMPLER:
    case BIND_TYPE_CB:
    case BIND_TYPE_COUNT:
    default: 
        break;
    }
  }

  void Texture::Bind(const Weak<CommandPair>& w_cmd, const Texture& dsv) const
  {
    const auto& cmd = w_cmd.lock();

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

    const D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle[]
    {
      m_rtv_->GetCPUDescriptorHandleForHeapStart()
    };

    const D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle[]
    {
      dsv.GetDSVDescriptor()->GetCPUDescriptorHandleForHeapStart()
    };

    cmd->GetList()->ResourceBarrier(1, &rtv_trans);
    cmd->GetList()->ResourceBarrier(1, &dsv_trans);
    cmd->GetList()->OMSetRenderTargets
      (
       1,
       rtv_handle,
       false,
       dsv_handle
      );
  }

  void Texture::Bind(const Weak<CommandPair>& w_cmd, Texture** rtvs, const UINT count, const Texture& dsv)
  {
    const auto& cmd = w_cmd.lock();
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv_handle;
    std::vector<D3D12_RESOURCE_BARRIER> transitions;

    rtv_handle.reserve(count);

    for (int i = 0; i < count; ++i)
    {
      const auto& rtv_trans = CD3DX12_RESOURCE_BARRIER::Transition
        (
         rtvs[i]->GetRawResource(),
         D3D12_RESOURCE_STATE_COMMON,
         D3D12_RESOURCE_STATE_RENDER_TARGET
        );

      rtv_handle.push_back(rtvs[i]->GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart());
      transitions.push_back(rtv_trans);
    }

    const auto& dsv_trans = CD3DX12_RESOURCE_BARRIER::Transition
      (
       dsv.GetRawResource(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_DEPTH_WRITE
      );

    transitions.push_back(dsv_trans);

    const D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle[]
    {
      dsv.GetDSVDescriptor()->GetCPUDescriptorHandleForHeapStart()
    };
    
    cmd->GetList()->ResourceBarrier(transitions.size(), transitions.data());

    cmd->GetList()->OMSetRenderTargets
      (
       count,
       rtv_handle.data(),
       false,
       dsv_handle
      );
  }

  void Texture::Unbind(const Weak<CommandPair>& cmd, const eBindType type) const
  {
    Unbind(cmd.lock()->GetList(), type);
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

    const auto& cmd = GetD3Device().AcquireCommandPair(L"Texture Mapping").lock();

    const auto dst = CD3DX12_TEXTURE_COPY_LOCATION(m_res_.Get(), 0);
    auto src = CD3DX12_TEXTURE_COPY_LOCATION(m_upload_buffer_.Get(), 0);
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = {0, {m_desc_.Format, m_desc_.Width, m_desc_.Height, m_desc_.DepthOrArraySize}};
    src.PlacedFootprint.Footprint.RowPitch = m_desc_.Width * DirectX::BitsPerPixel(m_desc_.Format) / 8;
    src.PlacedFootprint.Footprint.Depth = m_desc_.DepthOrArraySize;
    src.PlacedFootprint.Footprint.Width = m_desc_.Width;
    src.PlacedFootprint.Footprint.Height = m_desc_.Height;
    src.PlacedFootprint.Footprint.Format = m_desc_.Format;

    cmd->SoftReset();
    cmd->GetList()->ResourceBarrier(1, &dest_transition);
    cmd->GetList()->CopyTextureRegion
      (
       &dst,
       0, 0, 0,
       &src,
       nullptr
      );

    cmd->GetList()->ResourceBarrier(1, &dst_transition_back);
    cmd->FlagReady();
  }

  Texture::Texture()
    : Resource("", RES_T_TEX),
      m_desc_({}),
      m_type_(TEX_TYPE_2D),
      m_custom_desc_{false},
      m_b_lazy_window_(true) {}

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

  void Texture::Render(const float& dt) {}

  void Texture::PostRender(const float& dt) {}

  void Texture::PostUpdate(const float& dt) {}

  void Texture::InitializeDescriptorHeaps()
  {
    constexpr D3D12_DESCRIPTOR_HEAP_DESC buffer_desc = 
    {
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      1,
      D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
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
    if ((m_desc_.Flags & D3D11_BIND_DEPTH_STENCIL) && 
        (m_desc_.Flags & D3D11_BIND_UNORDERED_ACCESS))
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

      const auto& cmd = GetD3Device().AcquireCommandPair(L"Texture Uploading").lock();

      const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_res_.Get(),
         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
         D3D12_RESOURCE_STATE_COMMON
        );

      cmd->SoftReset();
      cmd->GetList()->ResourceBarrier(1, &common_transition);
      cmd->FlagReady();

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

      D3D12_CLEAR_VALUE clear_value = {};

      if (m_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
      {
        clear_value.Format = m_custom_desc_[0] ? m_rtv_desc_.Format : m_desc_.Format;
        clear_value.Color[0] = 0.0f;
        clear_value.Color[1] = 0.0f;
        clear_value.Color[2] = 0.0f;
        clear_value.Color[3] = 1.0f;
      }

      if (m_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
      {
        clear_value.Format = m_custom_desc_[1] ? m_dsv_desc_.Format : m_desc_.Format;
        clear_value.DepthStencil.Depth = 1.0f;
        clear_value.DepthStencil.Stencil = 0;
      }

      const D3D12_CLEAR_VALUE* cv_ptr = (
                                          m_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || 
                                        (m_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) ? &clear_value : nullptr;

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &heap_prop,
          D3D12_HEAP_FLAG_NONE,
          &desc,
          D3D12_RESOURCE_STATE_COMMON,
          cv_ptr,
          IID_PPV_ARGS(m_res_.GetAddressOf())
         )
        );
    }

    const auto name = GetName();

    if (name.empty())
    {
      DX::ThrowIfFailed(m_res_->SetName(L"Texture"));
    }
    else
    {
      const auto wname = L"Texture" + std::wstring(name.begin(), name.end());
      DX::ThrowIfFailed(m_res_->SetName(wname.c_str()));
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
          GetD3Device().GetCommandQueue(COMMAND_LIST_UPDATE), m_res_.Get(), false, image, D3D12_RESOURCE_STATE_COMMON,
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

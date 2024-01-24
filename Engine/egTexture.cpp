#include "pch.h"
#include "egTexture.h"
#include "egD3Device.hpp"
#include "egRenderPipeline.h"

SERIALIZER_ACCESS_IMPL(Engine::Resources::Texture, _ARTAG(_BSTSUPER(Resource)))

namespace Engine::Resources
{
  Texture::Texture(std::filesystem::path path, const eTexType type, const GenericTextureDescription& description)
    : Resource(std::move(path), RES_T_TEX),
      m_b_lazy_window_(true),
      m_desc_(description),
      m_type_(type),
      m_bind_to_(D3D11_BIND_SHADER_RESOURCE),
      m_bound_slot_(BIND_SLOT_TEX),
      m_bound_slot_offset_(0),
      m_bound_shader_(SHADER_PIXEL) {}

  eTexBindSlots Texture::GetSlot() const { return m_bound_slot_; }

  UINT Texture::GetSlotOffset() const { return m_bound_slot_offset_; }

  eShaderType Texture::GetBoundShader() const { return m_bound_shader_; }

  eTexType Texture::GetPrimitiveTextureType() const { return m_type_; }

  ID3D11ShaderResourceView* Texture::GetSRV() const { return m_srv_.Get(); }

  ID3D11RenderTargetView* Texture::GetRTV() const { return m_rtv_.Get(); }

  ID3D11DepthStencilView* Texture::GetDSV() const { return m_dsv_.Get(); }

  ID3D11UnorderedAccessView* Texture::GetUAV() const { return m_uav_.Get(); }

  bool Texture::IsHotload() const { return GetPath().empty(); }

  void Texture::BindAs(const D3D11_BIND_FLAG bind, const eTexBindSlots slot, const UINT slot_offset, const eShaderType shader)
  {
    m_bind_to_ = bind;
    m_bound_slot_   = slot;
    m_bound_slot_offset_ = slot_offset;
    m_bound_shader_ = shader;
  }

  void Texture::Map(const std::function<void(const D3D11_MAPPED_SUBRESOURCE&)>& copy_func) const
  {
    D3D11_MAPPED_SUBRESOURCE mapped{};
    GetD3Device().GetContext()->Map(m_res_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    copy_func(mapped);

    GetD3Device().GetContext()->Unmap(m_res_.Get(), 0);
  }

  Texture::Texture()
    : Resource("", RES_T_TEX),
      m_b_lazy_window_(true),
      m_desc_({}),
      m_type_(TEX_TYPE_2D),
      m_bind_to_(D3D11_BIND_SHADER_RESOURCE),
      m_bound_slot_(BIND_SLOT_TEX),
      m_bound_slot_offset_(0),
      m_bound_shader_(SHADER_PIXEL) {}

  UINT Texture::GetWidth() const { return m_desc_.Width; }

  UINT Texture::GetHeight() const { return m_desc_.Height; }

  UINT Texture::GetDepth() const { return m_desc_.Depth; }

  UINT Texture::GetArraySize() const { return m_desc_.ArraySize; }

  void Texture::LazyDescription(const  GenericTextureDescription& desc)
  {
    if (!m_b_lazy_window_)
    {
      throw std::runtime_error("Texture is already created with given description, Cannot initialize lazily");
    }

    m_desc_ = desc;
  }

  void Texture::LazyRTV(const   D3D11_RENDER_TARGET_VIEW_DESC& desc)
  {
    if (!m_b_lazy_window_)
    {
      throw std::runtime_error("Texture is already created with given description, Cannot initialize lazily");
    }

    m_custom_desc_.set(0);
    m_rtv_desc_ = desc;
  }

  void Texture::LazyDSV(const  D3D11_DEPTH_STENCIL_VIEW_DESC& desc)
  {
    if (!m_b_lazy_window_)
    {
      throw std::runtime_error("Texture is already created with given description, Cannot initialize lazily");
    }

    m_custom_desc_.set(1);
    m_dsv_desc_ = desc;
  }

  void Texture::LazyUAV(const  D3D11_UNORDERED_ACCESS_VIEW_DESC& desc)
  {
    if (!m_b_lazy_window_)
    {
      throw std::runtime_error("Texture is already created with given description, Cannot initialize lazily");
    }

    m_custom_desc_.set(2);
    m_uav_desc_ = desc;
  }

  void Texture::LazySRV(const  D3D11_SHADER_RESOURCE_VIEW_DESC& desc)
  {
    if (!m_b_lazy_window_)
    {
      throw std::runtime_error("Texture is already created with given description, Cannot initialize lazily");
    }

    m_custom_desc_.set(3);
    m_srv_desc_ = desc;
  }

  const Texture::GenericTextureDescription& Texture::GetDescription() const { return m_desc_; }

  Texture::~Texture() {}

  void Texture::Initialize() {}

  void Texture::PreUpdate(const float& dt) {}

  void Texture::Update(const float& dt) {}

  void Texture::PreRender(const float& dt) {}

  void Texture::Render(const float& dt)
  {
    if (m_bind_to_ == D3D11_BIND_RENDER_TARGET)
    {
      GetD3Device().GetContext()->OMGetRenderTargets(1, s_previous_rtv.GetAddressOf(), s_previous_dsv.GetAddressOf());
      GetD3Device().GetContext()->OMSetRenderTargets(1, m_rtv_.GetAddressOf(), nullptr);
    }
    else if (m_bind_to_ == D3D11_BIND_SHADER_RESOURCE)
    {
      switch (m_bound_shader_)
      {
      case SHADER_VERTEX:
          GetD3Device().GetContext()->VSSetShaderResources(m_bound_slot_ + m_bound_slot_offset_, 1, m_srv_.GetAddressOf());
          break;
      case SHADER_PIXEL:
          GetD3Device().GetContext()->PSSetShaderResources(m_bound_slot_ + m_bound_slot_offset_, 1, m_srv_.GetAddressOf());
          break;
      case SHADER_GEOMETRY:
          GetD3Device().GetContext()->GSSetShaderResources(m_bound_slot_ + m_bound_slot_offset_, 1, m_srv_.GetAddressOf());
          break;
      case SHADER_COMPUTE:
          GetD3Device().GetContext()->CSSetShaderResources(m_bound_slot_ + m_bound_slot_offset_, 1, m_srv_.GetAddressOf());
          break;
      case SHADER_HULL:
          GetD3Device().GetContext()->HSSetShaderResources(m_bound_slot_ + m_bound_slot_offset_, 1, m_srv_.GetAddressOf());
          break;
      case SHADER_DOMAIN:
          GetD3Device().GetContext()->DSSetShaderResources(m_bound_slot_ + m_bound_slot_offset_, 1, m_srv_.GetAddressOf());
          break;
      case SHADER_UNKNOWN:
      default: break;
      }
    }
    else if (m_bind_to_ == D3D11_BIND_DEPTH_STENCIL)
    {
      ComPtr<ID3D11RenderTargetView> null_view = nullptr;
      GetD3Device().GetContext()->OMGetRenderTargets(1, s_previous_rtv.GetAddressOf(), s_previous_dsv.GetAddressOf());
      GetD3Device().GetContext()->OMSetRenderTargets(1, null_view.GetAddressOf(), m_dsv_.Get());
    }
    else if (m_bind_to_ == D3D11_BIND_UNORDERED_ACCESS)
    {
      GetD3Device().GetContext()->CSSetUnorderedAccessViews(m_bound_slot_ + m_bound_slot_offset_, 1, m_uav_.GetAddressOf(), nullptr);
    }
  }

  void Texture::PostRender(const float& dt)
  {
    if (m_bind_to_ == D3D11_BIND_RENDER_TARGET)
    {
      GetD3Device().GetContext()->OMSetRenderTargets(1, s_previous_rtv.GetAddressOf(), s_previous_dsv.Get());
    }
    else if (m_bind_to_ == D3D11_BIND_SHADER_RESOURCE)
    {
      ComPtr<ID3D11ShaderResourceView> null_view = nullptr;

      switch (m_bound_shader_)
      {
      case SHADER_VERTEX:
          GetD3Device().GetContext()->VSSetShaderResources(m_bound_slot_ + m_bound_slot_offset_, 1, null_view.GetAddressOf());
          break;
      case SHADER_PIXEL:
          GetD3Device().GetContext()->PSSetShaderResources(m_bound_slot_ + m_bound_slot_offset_, 1, null_view.GetAddressOf());
          break;
      case SHADER_GEOMETRY:
          GetD3Device().GetContext()->GSSetShaderResources(m_bound_slot_ + m_bound_slot_offset_, 1, null_view.GetAddressOf());
          break;
      case SHADER_HULL:
          GetD3Device().GetContext()->HSSetShaderResources(m_bound_slot_ + m_bound_slot_offset_, 1, null_view.GetAddressOf());
          break;
      case SHADER_DOMAIN:
          GetD3Device().GetContext()->DSSetShaderResources(m_bound_slot_ + m_bound_slot_offset_, 1, null_view.GetAddressOf());
          break;
      case SHADER_UNKNOWN: 
      default: break;
      }
    }
    else if (m_bind_to_ == D3D11_BIND_UNORDERED_ACCESS)
    {
      ComPtr<ID3D11UnorderedAccessView> null_view = nullptr;

      GetD3Device().GetContext()->CSSetUnorderedAccessViews(m_bound_slot_ + m_bound_slot_offset_, 1, null_view.GetAddressOf(), nullptr);
    }
    else if (m_bind_to_ == D3D11_BIND_DEPTH_STENCIL)
    {
      GetD3Device().GetContext()->OMSetRenderTargets(1, s_previous_rtv.GetAddressOf(), s_previous_dsv.Get());
    }
  }

  void Texture::PostUpdate(const float& dt) {}

  void Texture::Load_INTERNAL()
  {
    if (!GetPath().empty() && (m_desc_.Width + m_desc_.Height + m_desc_.Depth))
    {
      throw std::logic_error("While loading from file, width height depth does not needed");
    }

    if ((m_desc_.BindFlags & D3D11_BIND_DEPTH_STENCIL) && (m_desc_.BindFlags & D3D11_BIND_UNORDERED_ACCESS))
    {
      throw std::logic_error("Depth stencil and unordered cannot be flagged in same texture");
    }

    if (!GetPath().empty())
    {
      const UINT flag = m_desc_.BindFlags;

      GetD3Device().CreateTextureFromFile
      (
       absolute(GetPath()),
       flag,
       m_res_.ReleaseAndGetAddressOf(), 
       m_srv_.ReleaseAndGetAddressOf()
      );

      ComPtr<ID3D11Texture1D> t_1d;
      ComPtr<ID3D11Texture2D> t_2d;
      ComPtr<ID3D11Texture3D> t_3d;

      // casting test
      if (m_res_.As(&t_1d) == S_OK)
      {
        D3D11_TEXTURE1D_DESC desc;
        t_1d->GetDesc(&desc);

        m_type_ = TEX_TYPE_1D;
        m_desc_.Format = desc.Format;
        m_desc_.ArraySize = desc.ArraySize;
        m_desc_.Width = desc.Width;
        m_desc_.Height = 0;
        m_desc_.Depth = 0;
      }
      else if (m_res_.As(&t_2d) == S_OK)
      {
        D3D11_TEXTURE2D_DESC desc{};
        t_2d->GetDesc(&desc);

        m_type_ = TEX_TYPE_2D;
        m_desc_.Format = desc.Format;
        m_desc_.ArraySize = desc.ArraySize;
        m_desc_.Width = desc.Width;
        m_desc_.Height = desc.Height;
        m_desc_.Depth = 0;
      }
      else if (m_res_.As(&t_3d) == S_OK)
      {
        D3D11_TEXTURE3D_DESC desc{};
        t_3d->GetDesc(&desc);

        m_type_ = TEX_TYPE_3D;
        m_desc_.Format = desc.Format;
        m_desc_.ArraySize = 1; // undefined
        m_desc_.Width = desc.Width;
        m_desc_.Height = desc.Height;
        m_desc_.Depth = desc.Depth;
      }
      else
      {
        throw std::runtime_error("Unknown type is loaded into texture");
      }
    }
    else
    {
      loadDerived(m_res_);
    }

    if (m_desc_.BindFlags & D3D11_BIND_SHADER_RESOURCE && !m_srv_)
    {
      if (m_custom_desc_.test(3))
      {
        DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateShaderResourceView(m_res_.Get(), &m_srv_desc_, m_srv_.ReleaseAndGetAddressOf()));
      }
      else
      {
        DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateShaderResourceView(m_res_.Get(), nullptr, m_srv_.ReleaseAndGetAddressOf()));
      }
    }

    if (m_desc_.BindFlags & D3D11_BIND_RENDER_TARGET)
    {
      if (m_custom_desc_.test(0))
      {
        DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateRenderTargetView(m_res_.Get(), &m_rtv_desc_, m_rtv_.ReleaseAndGetAddressOf()));
      }
      else
      {
        DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateRenderTargetView(m_res_.Get(), nullptr, m_rtv_.ReleaseAndGetAddressOf()));
      }
    }

    if (m_desc_.BindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
      if (m_custom_desc_.test(1))
      {
        DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateDepthStencilView(m_res_.Get(), &m_dsv_desc_, m_dsv_.ReleaseAndGetAddressOf()));
      }
      else
      {
        DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateDepthStencilView(m_res_.Get(), nullptr, m_dsv_.ReleaseAndGetAddressOf()));
      }
    }

    if (m_desc_.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
    {
      if (m_custom_desc_.test(1))
      {
        DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateUnorderedAccessView(m_res_.Get(), &m_uav_desc_, m_uav_.ReleaseAndGetAddressOf()));
      }
      else
      {
        DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateUnorderedAccessView(m_res_.Get(), nullptr, m_uav_.ReleaseAndGetAddressOf()));
      }
    }

    m_b_lazy_window_ = false;
  }

  void Texture::Unload_INTERNAL()
  {
    m_srv_.Reset();
    m_rtv_.Reset();
    m_dsv_.Reset();
    m_uav_.Reset();
    m_res_.Reset();
  }

  void Texture::FixedUpdate(const float& dt) {}

  eResourceType Texture::GetResourceType() const { return Resource::GetResourceType(); }
} // namespace Engine::Resources

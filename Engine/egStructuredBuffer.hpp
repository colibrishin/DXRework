#pragma once
#include "Windows.h"
#include "egType.h"

namespace Engine::Graphics
{
  template <typename T>
  class StructuredBuffer
  {
  public:
    StructuredBuffer();
    ~StructuredBuffer() = default;

    void            Create(UINT size, const T* initial_data, bool is_mutable = false);
    void __fastcall SetData(UINT size, const T* src_ptr);
    void __fastcall GetData(UINT size, T* dst_ptr);
    void            BindSRV(eShaderType shader);
    void            UnbindSRV(eShaderType shader);
    void            Clear();

    template <typename U = T, typename std::enable_if_t<is_uav_sb<U>::value, bool> = true>
    void BindUAV()
    {
      static_assert(is_uav_sb<T>::value == true, "It is not defined in UAV structured buffer");

      if (m_b_srv_bound_) { throw std::logic_error("StructuredBuffer is already bound as SRV"); }

      if constexpr (is_client_uav_sb<T>::value == true)
      {
        GetD3Device().GetContext()->CSSetUnorderedAccessViews
          (
           which_client_sb<T>::value,
           1,
           m_uav_.GetAddressOf(),
           nullptr
          );
      }
      if constexpr (is_uav_sb<T>::value == true)
      {
        GetD3Device().GetContext()->CSSetUnorderedAccessViews
        (
         which_sb_uav<T>::value,
         1,
         m_uav_.GetAddressOf(),
         nullptr
        );
      }

      m_b_uav_bound_ = true;
    }

    template <typename U = T, typename std::enable_if_t<is_uav_sb<U>::value, bool> = true>
    void UnbindUAV()
    {
      static_assert(is_uav_sb<T>::value == true, "It is not defined in UAV structured buffer");

      ComPtr<ID3D11UnorderedAccessView> null_uav = nullptr;

      if constexpr (is_client_uav_sb<T>::value == true)
      {
        GetD3Device().GetContext()->CSSetUnorderedAccessViews
          (
           which_client_sb<T>::value,
           1,
           null_uav.GetAddressOf(),
           nullptr
          );

      }
      if constexpr (is_uav_sb<T>::value == true)
      {
        GetD3Device().GetContext()->CSSetUnorderedAccessViews
        (
         which_sb_uav<T>::value,
         1,
         null_uav.GetAddressOf(),
         nullptr
        );
      }

      m_b_uav_bound_ = false;
    }

  private:
    void InitializeSRV(UINT size);
    void InitializeUAV(UINT size);
    void InitializeMainBuffer(UINT size, const T* initial_data);
    void InitializeWriteBuffer(UINT size);
    void InitializeReadBuffer(UINT size);

    bool        m_b_srv_bound_;
    bool        m_b_uav_bound_;
    bool        m_b_mutable_;
    UINT        m_size_;

    ComPtr<ID3D11Buffer> m_buffer_;

    ComPtr<ID3D11ShaderResourceView>  m_srv_;
    ComPtr<ID3D11UnorderedAccessView> m_uav_;

    ComPtr<ID3D11Buffer> m_write_buffer_;
    ComPtr<ID3D11Buffer> m_read_buffer_;
  };

  template <typename T>
  void StructuredBuffer<T>::InitializeSRV(UINT size)
  {
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    srv_desc.Format               = DXGI_FORMAT_UNKNOWN;
    srv_desc.ViewDimension        = D3D11_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.ElementOffset = 0;
    srv_desc.Buffer.ElementWidth  = size;

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateShaderResourceView(m_buffer_.Get(), &srv_desc, m_srv_.ReleaseAndGetAddressOf())
      );
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeUAV(UINT size)
  {
    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    uav_desc.Format              = DXGI_FORMAT_UNKNOWN;
    uav_desc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.Flags        = 0;
    uav_desc.Buffer.NumElements  = size;

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateUnorderedAccessView(m_buffer_.Get(), &uav_desc, m_uav_.ReleaseAndGetAddressOf())
      );
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeMainBuffer(UINT size, const T* initial_data)
  {
    if ((sizeof(T) * size) % 16 != 0) { throw std::runtime_error("StructuredBuffer size need to be dividable by 16"); }

    D3D11_BUFFER_DESC desc;

    UINT bind_flags = D3D11_BIND_SHADER_RESOURCE;

    if constexpr (is_uav_sb<T>::value == true)
    {
      bind_flags |= D3D11_BIND_UNORDERED_ACCESS;
    }

    desc.BindFlags           = bind_flags;
    desc.ByteWidth           = sizeof(T) * size;
    desc.CPUAccessFlags      = 0;
    desc.StructureByteStride = sizeof(T);
    desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.Usage               = D3D11_USAGE_DEFAULT;

    if (initial_data != nullptr)
    {
      D3D11_SUBRESOURCE_DATA data;
      data.pSysMem = initial_data;
      DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateBuffer(&desc, &data, m_buffer_.GetAddressOf()));
    }
    else
    {
      DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateBuffer(&desc, nullptr, m_buffer_.GetAddressOf()));
    }
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeWriteBuffer(UINT size)
  {
    D3D11_BUFFER_DESC w_desc;
    w_desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    w_desc.ByteWidth           = sizeof(T) * size;
    w_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    w_desc.StructureByteStride = sizeof(T);
    w_desc.MiscFlags           = 0;
    w_desc.Usage               = D3D11_USAGE_DYNAMIC;

    DX::ThrowIfFailed
      (GetD3Device().GetDevice()->CreateBuffer(&w_desc, nullptr, m_write_buffer_.ReleaseAndGetAddressOf()));
  }

  template <typename T>
  StructuredBuffer<T>::StructuredBuffer()
    : m_b_srv_bound_(false),
      m_b_uav_bound_(false),
      m_b_mutable_(false),
      m_size_(0)
  {
    static_assert(sizeof(T) <= 2048, "StructuredBuffer struct T size is too big");
    static_assert(sizeof(T) % sizeof(Vector4) == 0, "StructuredBuffer struct T size need to be dividable by 16");
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeReadBuffer(UINT size)
  {
    D3D11_BUFFER_DESC r_desc;
    r_desc.BindFlags           = 0;
    r_desc.ByteWidth           = sizeof(T) * size;
    r_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
    r_desc.StructureByteStride = sizeof(T);
    r_desc.MiscFlags           = 0;
    r_desc.Usage               = D3D11_USAGE_STAGING;

    DX::ThrowIfFailed
      (GetD3Device().GetDevice()->CreateBuffer(&r_desc, nullptr, m_read_buffer_.ReleaseAndGetAddressOf()));
  }

  template <typename T>
  void StructuredBuffer<T>::Create(UINT size, const T* initial_data, bool is_mutable)
  {
    m_b_mutable_ = is_mutable;
    m_size_      = size;

    InitializeMainBuffer(size, initial_data);
    InitializeSRV(size);
    if constexpr (is_uav_sb<T>::value == true)
    {
      InitializeUAV(size);
    }

    if (is_mutable) { InitializeWriteBuffer(size); }

    InitializeReadBuffer(size);
  }

  template <typename T>
  void StructuredBuffer<T>::SetData(UINT size, const T* src_ptr)
  {
    if (!m_b_mutable_) { throw std::logic_error("StructuredBuffer is defined as not mutable"); }

    if (m_size_ < size) { Create(size, nullptr, m_b_mutable_); }

    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    GetD3Device().GetContext()->Map(m_write_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);

    std::memcpy(mapped_resource.pData, src_ptr, sizeof(T) * size);

    GetD3Device().GetContext()->Unmap(m_write_buffer_.Get(), 0);

    GetD3Device().GetContext()->CopyResource(m_buffer_.Get(), m_write_buffer_.Get());
  }

  template <typename T>
  void StructuredBuffer<T>::GetData(UINT size, T* dst_ptr)
  {
    GetD3Device().GetContext()->CopyResource(m_read_buffer_.Get(), m_buffer_.Get());

    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    GetD3Device().GetContext()->Map(m_read_buffer_.Get(), 0, D3D11_MAP_READ, 0, &mapped_resource);
    std::memcpy(dst_ptr, mapped_resource.pData, sizeof(T) * size);
    GetD3Device().GetContext()->Unmap(m_read_buffer_.Get(), 0);
  }

  template <typename T>
  void StructuredBuffer<T>::BindSRV(const eShaderType shader)
  {
    if (m_b_uav_bound_) { throw std::logic_error("StructuredBuffer is already bound as UAV"); }

    if constexpr (is_client_sb<T>::value == true)
    {
      GetRenderPipeline().BindResource(which_client_sb<T>::value, shader, m_srv_.GetAddressOf());
    }
    else
    {
      GetRenderPipeline().BindResource(which_sb<T>::value, shader, m_srv_.GetAddressOf());
    }

    m_b_srv_bound_ = true;
  }

  template <typename T>
  void StructuredBuffer<T>::UnbindSRV(const eShaderType shader)
  {
    if constexpr (is_client_sb<T>::value == true)
    {
      GetRenderPipeline().UnbindResource(which_client_sb<T>::value, shader);
    }
    else
    {
      GetRenderPipeline().UnbindResource(which_sb<T>::value, shader);
    }

    m_b_srv_bound_ = false;
  }

  template <typename T>
  void StructuredBuffer<T>::Clear()
  {
    m_buffer_.Reset();
    m_srv_.Reset();
    m_write_buffer_.Reset();
    m_read_buffer_.Reset();
  }
}

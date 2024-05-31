#pragma once
#include <BufferHelpers.h>
#include <d3dx12.h>

#include "Windows.h"
#include "egCommon.hpp"
#include "egType.h"

namespace Engine::Graphics
{
  inline static std::vector<ComPtr<ID3D12Resource>> g_sb_upload_buffers = {};

  static void _reset_structured_buffer()
  {
    for (auto& buffer : g_sb_upload_buffers)
    {
      buffer.Reset();
    }

    g_sb_upload_buffers.clear();
  }

  template <typename T>
  class StructuredBuffer
  {
  public:
    StructuredBuffer();
    ~StructuredBuffer() = default;

    void            Create(UINT size, const T* initial_data, bool is_mutable = false);
    void __fastcall SetDataDeferred(UINT size, const T* src_ptr);
    void __fastcall GetData(UINT size, T* dst_ptr);
    void            Clear();

    void BindSRVDeferred();
    void UnbindSRVDeferred();

    template <typename U = T, typename std::enable_if_t<is_uav_sb<U>::value, bool> = true>
    void BindUAVDeferred()
    {
      if (m_b_srv_bound_)
      {
        throw std::logic_error("StructuredBuffer is bound as SRV, cannot bind as UAV");
      }

      if (m_b_uav_bound_) { return; }

      DirectCommandGuard dcg;

      const auto& uav_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COMMON,
         D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        );

      GetD3Device().GetCommandList()->ResourceBarrier(1, &uav_transition);

      m_b_uav_bound_ = true;
    }

    template <typename U = T, typename std::enable_if_t<is_uav_sb<U>::value, bool> = true>
    void UnbindUAVDeferred()
    {
      if (!m_b_uav_bound_) { return; }

      DirectCommandGuard dcg;

      const auto& uav_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
         D3D12_RESOURCE_STATE_COMMON
        );

      GetD3Device().GetCommandList()->ResourceBarrier(1, &uav_transition);

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

    bool        m_b_srv_bound_compute_;
    bool        m_b_uav_bound_compute_;

    bool        m_b_mutable_;
    UINT        m_size_;

    ComPtr<ID3D12Resource> m_buffer_;
    ComPtr<ID3D12Resource> m_write_buffer_;
    ComPtr<ID3D12Resource> m_read_buffer_;
  };

  template <typename T>
  void StructuredBuffer<T>::InitializeSRV(const UINT size)
  {
    const D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc
    {
      .Format = DXGI_FORMAT_UNKNOWN,
      .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
      .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
      .Buffer =
      {
        .FirstElement = 0,
        .NumElements = size,
        .StructureByteStride = sizeof(T),
        .Flags = D3D12_BUFFER_SRV_FLAG_NONE
      }
    };

    auto srv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(GetRenderPipeline().GetBufferHeap()->GetCPUDescriptorHandleForHeapStart());

    srv_handle.Offset(static_cast<INT>(CB_TYPE_END + BIND_SLOT_UAV_END + which_sb<T>::value), GetRenderPipeline().GetBufferDescriptorSize());

    GetD3Device().GetDevice()->CreateShaderResourceView
     (
       m_buffer_.Get(),
       &srv_desc,
       srv_handle
     );
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeUAV(UINT size)
  {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    uav_desc.Format              = DXGI_FORMAT_UNKNOWN;
    uav_desc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements  = size;

    auto uav_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(GetRenderPipeline().GetBufferHeap()->GetCPUDescriptorHandleForHeapStart());

    if constexpr (is_client_sb<T>::value == true)
    {
      uav_handle.Offset(static_cast<INT>(CB_TYPE_END + which_client_sb_uav<T>::value), GetRenderPipeline().GetBufferDescriptorSize());
    }
    else
    {
      uav_handle.Offset(static_cast<INT>(CB_TYPE_END + which_sb_uav<T>::value), GetRenderPipeline().GetBufferDescriptorSize());
    }

    GetD3Device().GetDevice()->CreateUnorderedAccessView
      (
       m_buffer_.Get(),
       nullptr,
       &uav_desc,
       uav_handle
      );
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeMainBuffer(UINT size, const T* initial_data)
  {
    if ((sizeof(T) * size) % 16 != 0)
    {
      throw std::runtime_error("StructuredBuffer size need to be dividable by 16");
    }

    const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    if (initial_data != nullptr)
    {
      const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * size);

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &default_heap,
          D3D12_HEAP_FLAG_NONE,
          &buffer_desc,
          D3D12_RESOURCE_STATE_COPY_DEST,
          nullptr,
          IID_PPV_ARGS(m_buffer_.ReleaseAndGetAddressOf())
         )
        );

      // Use upload buffer to transfer data to the default buffer
      // Note that resource is copied if command list executed,
      // therefore it is not executed immediately.

      DX::ThrowIfFailed
        (
         DirectX::CreateUploadBuffer
         (
          GetD3Device().GetDevice(),
          initial_data,
          size,
          m_write_buffer_.GetAddressOf()
         )
        );

      char* data = nullptr;

      DX::ThrowIfFailed(m_write_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));

      std::memcpy(data, initial_data, sizeof(T) * size);

      m_write_buffer_->Unmap(0, nullptr);

      {
        DirectCommandGuard cg;

        const auto& end_state = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_COMMON
        );

         GetD3Device().GetCommandList()->CopyResource
        (
            m_buffer_.Get(), 
            m_write_buffer_.Get()
        );

        GetD3Device().GetCommandList()->ResourceBarrier(1, &end_state);
      }
    }
    else
    {
      const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * size);

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &default_heap,
          D3D12_HEAP_FLAG_NONE,
          &buffer_desc,
          D3D12_RESOURCE_STATE_COMMON,
          nullptr,
          IID_PPV_ARGS(m_buffer_.ReleaseAndGetAddressOf())
         )
        );
    }
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeWriteBuffer(const UINT size)
  {
    if (m_write_buffer_ != nullptr) { return; }

    const auto& upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * size);

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommittedResource
       (
        &upload_heap,
        D3D12_HEAP_FLAG_NONE,
        &buffer_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_write_buffer_.ReleaseAndGetAddressOf())
       )
      );
  }

  template <typename T>
  StructuredBuffer<T>::StructuredBuffer()
    : m_b_srv_bound_(false),
      m_b_uav_bound_(false),
      m_b_srv_bound_compute_(false),
      m_b_uav_bound_compute_(false),
      m_b_mutable_(false),
      m_size_(0)
  {
    static_assert(sizeof(T) <= 2048, "StructuredBuffer struct T size is too big");
    static_assert(sizeof(T) % sizeof(Vector4) == 0, "StructuredBuffer struct T size need to be dividable by 16");
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeReadBuffer(const UINT size)
  {
    const auto& readback_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

    const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * size);


    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommittedResource
       (
        &readback_heap,
        D3D12_HEAP_FLAG_NONE,
        &buffer_desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(m_read_buffer_.ReleaseAndGetAddressOf())
       )
      );
  }

  template <typename T>
  void StructuredBuffer<T>::BindSRVDeferred()
  {
    if (m_b_uav_bound_)
    {
      throw std::logic_error("StructuredBuffer is bound as UAV, cannot bind as SRV");
    }

    if (m_b_srv_bound_) { return; }

    DirectCommandGuard dcg;

    const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
      );

    GetD3Device().GetCommandList()->ResourceBarrier(1, &srv_transition);

    m_b_srv_bound_ = true;
  }

  template <typename T>
  void StructuredBuffer<T>::UnbindSRVDeferred()
  {
    if (!m_b_srv_bound_) { return; }

    DirectCommandGuard dcg;

    const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
       D3D12_RESOURCE_STATE_COMMON
      );

    GetD3Device().GetCommandList()->ResourceBarrier(1, &srv_transition);

    m_b_srv_bound_ = false;
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

    if (is_mutable)
    {
      InitializeWriteBuffer(size);
      std::vector<T> data(size);
      SetDataDeferred(size, data.data());
    }
    InitializeReadBuffer(size);
  }

  template <typename T>
  void StructuredBuffer<T>::SetDataDeferred(const UINT size, const T* src_ptr)
  {
    if (!m_b_mutable_) { throw std::logic_error("StructuredBuffer is defined as not mutable"); }

    if (m_size_ < size) { Create(size, nullptr, m_b_mutable_); }

    ComPtr<ID3D12Resource> upload_buffer;

    DX::ThrowIfFailed
      (
       DirectX::CreateUploadBuffer
       (
        GetD3Device().GetDevice(),
        src_ptr,
        size,
        upload_buffer.GetAddressOf()
       )
      );

    char* data = nullptr;

    DX::ThrowIfFailed(upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&data)));

    std::memcpy(data, src_ptr, sizeof(T) * size);

    upload_buffer->Unmap(0, nullptr);

    g_sb_upload_buffers.push_back(upload_buffer);

    {
      DirectCommandGuard cg;

      D3D12_RESOURCE_STATES before_state = D3D12_RESOURCE_STATE_COMMON;

      if (m_b_srv_bound_)
      {
        before_state = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
      }
      if (m_b_uav_bound_)
      {
        before_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      }

      const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       before_state,
       D3D12_RESOURCE_STATE_COPY_DEST
      );
    
      GetD3Device().GetCommandList()->ResourceBarrier(1, &barrier);

      GetD3Device().GetCommandList()->CopyResource
        (
         m_buffer_.Get(),
         upload_buffer.Get()
        );

      const auto& revert_barrier = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COPY_DEST,
         before_state
        );
    
      GetD3Device().GetCommandList()->ResourceBarrier(1, &revert_barrier);
    }
  }

  // This will execute the command list and copy the data from the GPU to the CPU.
  template <typename T>
  void StructuredBuffer<T>::GetData(const UINT size, T* dst_ptr)
  {
    D3D12_RESOURCE_STATES before_state = D3D12_RESOURCE_STATE_COMMON;

    if (m_b_srv_bound_)
    {
      before_state = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
    }

    if (m_b_uav_bound_)
    {
      before_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       before_state,
       D3D12_RESOURCE_STATE_COPY_SOURCE
      );

    GetD3Device().GetCopyCommandList()->ResourceBarrier(1, &barrier);

    GetD3Device().GetCopyCommandList()->CopyResource
      (
       m_read_buffer_.Get(),
       m_buffer_.Get()
      );

    const auto& revert_barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COPY_SOURCE,
       before_state
      );

    GetD3Device().GetCopyCommandList()->ResourceBarrier(1, &revert_barrier);

    GetD3Device().ExecuteCopyCommandList();

    char* data = nullptr;

    DX::ThrowIfFailed(m_read_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));

    std::memcpy(dst_ptr, data, sizeof(T) * size);

    m_read_buffer_->Unmap(0, nullptr);
  }

  template <typename T>
  void StructuredBuffer<T>::Clear()
  {
    m_buffer_.Reset();
    m_write_buffer_.Reset();
    m_read_buffer_.Reset();
  }

  template <typename T>
  void StructuredBuffer<T>::BindSRVComputeDeferred()
  {
    if (m_b_uav_bound_ || m_b_uav_bound_compute_)
    {
      throw std::logic_error("StructuredBuffer is bound as UAV, cannot bind as SRV");
    }

    if (m_b_srv_bound_ || m_b_srv_bound_compute_) { return; }

    const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
      );

    GetD3Device().GetComputeCommandList()->ResourceBarrier(1, &srv_transition);

    m_b_srv_bound_compute_ = true;
  }

  template <typename T>
  void StructuredBuffer<T>::UnbindSRVComputeDeferred()
  {
    if (!m_b_srv_bound_compute_) { return; }

    const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
          D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
       D3D12_RESOURCE_STATE_COMMON
      );

    GetD3Device().GetComputeCommandList()->ResourceBarrier(1, &srv_transition);

    m_b_srv_bound_compute_ = false;
  }
}

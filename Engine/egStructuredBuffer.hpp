#pragma once
#include <BufferHelpers.h>
#include <d3dx12.h>

#include "Windows.h"
#include "egCommon.hpp"
#include "egGarbageCollector.h"
#include "egType.h"

namespace Engine::Graphics
{
  template <typename T>
  class StructuredBuffer
  {
  public:
    StructuredBuffer();
    ~StructuredBuffer() = default;

    void            Create(const eCommandList list, UINT size, const T * initial_data, bool is_mutable = false);
    void __fastcall SetData(const eCommandList list, const UINT size, const T * src_ptr);
    void __fastcall GetData(UINT size, T* dst_ptr);
    void            Clear();

    void BindSRVGraphic(const eCommandList list);
    void UnbindSRVGraphic(const eCommandList list);

    template <typename U = T, typename std::enable_if_t<is_uav_sb<U>::value, bool> = true>
    void BindUAVGraphic(const eCommandList list)
    {
      if (m_b_srv_bound_)
      {
        throw std::logic_error("StructuredBuffer is bound as SRV, cannot bind as UAV");
      }

      if (!m_b_uav_bound_) { return; }

      const auto& uav_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COMMON,
         D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        );

      GetD3Device().GetCommandList(list)->ResourceBarrier(1, &uav_transition);

      if constexpr (is_client_uav_sb<T>::value == true)
      {
        GetRenderPipeline().GetDescriptor().SetUnorderedAccess
          (
           m_uav_heap_->GetCPUDescriptorHandleForHeapStart(),
           which_client_sb_uav<T>::value
          );
      }
      else if constexpr (is_uav_sb<T>::value == true)
      {
        GetRenderPipeline().GetDescriptor().SetUnorderedAccess
          (
           m_uav_heap_->GetCPUDescriptorHandleForHeapStart(),
           which_sb_uav<T>::value
          );
      }

      m_b_uav_bound_ = true;
    }

    template <typename U = T, typename std::enable_if_t<is_uav_sb<U>::value, bool> = true>
    void UnbindUAVGraphic(const eCommandList list)
    {
      if (!m_b_uav_bound_) { return; }

      const auto& uav_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
         D3D12_RESOURCE_STATE_COMMON
        );

      GetD3Device().GetCommandList(list)->ResourceBarrier(1, &uav_transition);

      m_b_uav_bound_ = false;
    }

  private:
    void InitializeSRV(UINT size);
    void InitializeUAV(UINT size);
    void InitializeMainBuffer(UINT size, const T * initial_data);
    void InitializeWriteBuffer(UINT size);
    void InitializeReadBuffer(UINT size);

    bool        m_b_srv_bound_;
    bool        m_b_uav_bound_;

    bool        m_b_mutable_;
    UINT        m_size_;

    ComPtr<ID3D12DescriptorHeap> m_srv_heap_;
    ComPtr<ID3D12DescriptorHeap> m_uav_heap_;

    ComPtr<ID3D12Resource> m_buffer_;
    ComPtr<ID3D12Resource> m_write_buffer_;
    ComPtr<ID3D12Resource> m_read_buffer_;
  };

  template <typename T>
  void StructuredBuffer<T>::InitializeSRV(const UINT size)
  {
    constexpr D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &srv_heap_desc,
        IID_PPV_ARGS(m_srv_heap_.ReleaseAndGetAddressOf())
       )
      );

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

    GetD3Device().GetDevice()->CreateShaderResourceView
     (
       m_buffer_.Get(),
       &srv_desc,
       m_srv_heap_->GetCPUDescriptorHandleForHeapStart()
     );
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeUAV(UINT size)
  {
    constexpr D3D12_DESCRIPTOR_HEAP_DESC uav_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &uav_heap_desc,
        IID_PPV_ARGS(m_uav_heap_.ReleaseAndGetAddressOf())
       )
      );

    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    uav_desc.Format              = DXGI_FORMAT_UNKNOWN;
    uav_desc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements  = size;
    uav_desc.Buffer.StructureByteStride = sizeof(T);
    uav_desc.Buffer.CounterOffsetInBytes = 0;
    uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    GetD3Device().GetDevice()->CreateUnorderedAccessView
      (
       m_buffer_.Get(),
       nullptr,
       &uav_desc,
       m_uav_heap_->GetCPUDescriptorHandleForHeapStart()
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

    auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * size);

    if constexpr (is_uav_sb<T>::value == true)
    {
      buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    const std::string gen_type_name = typeid(T).name();
    const std::wstring type_name(gen_type_name.begin(), gen_type_name.end());
    const std::wstring buffer_name = L"StructuredBuffer " + type_name;

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

    DX::ThrowIfFailed(m_buffer_->SetName(buffer_name.c_str()));

    if (initial_data != nullptr)
    {
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

      const std::wstring write_buffer_name = L"StructuredBuffer Write " + type_name;

      DX::ThrowIfFailed(m_write_buffer_->SetName(write_buffer_name.c_str()));

      char* data = nullptr;

      DX::ThrowIfFailed(m_write_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));

      std::memcpy(data, initial_data, sizeof(T) * size);

      m_write_buffer_->Unmap(0, nullptr);

      const auto& dest_state = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_COPY_DEST
      );

      GetD3Device().WaitAndReset(COMMAND_LIST_COPY);

      GetD3Device().GetCommandList(COMMAND_LIST_COPY)->ResourceBarrier(1, &dest_state);

      GetD3Device().GetCommandList(COMMAND_LIST_COPY)->CopyResource
        (
         m_buffer_.Get(),
         m_write_buffer_.Get()
        );

      const auto& end_state = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COPY_DEST,
       D3D12_RESOURCE_STATE_COMMON
      );

      GetD3Device().GetCommandList(COMMAND_LIST_COPY)->ResourceBarrier(1, &end_state);

      GetD3Device().ExecuteCommandList(COMMAND_LIST_COPY);

    }
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeWriteBuffer(const UINT size)
  {
    if (m_write_buffer_ != nullptr) { return; }

    const auto& upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * size);

    const std::string gen_type_name = typeid(T).name();
    const std::wstring type_name(gen_type_name.begin(), gen_type_name.end());
    const std::wstring buffer_name = L"StructuredBuffer Write " + type_name;

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

    DX::ThrowIfFailed(m_write_buffer_->SetName(buffer_name.c_str()));
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
  void StructuredBuffer<T>::InitializeReadBuffer(const UINT size)
  {
    const auto& readback_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * size);

    const std::string gen_type_name = typeid(T).name();
    const std::wstring type_name(gen_type_name.begin(), gen_type_name.end());
    const std::wstring buffer_name = L"StructuredBuffer Read " + type_name;

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

    DX::ThrowIfFailed(m_read_buffer_->SetName(buffer_name.c_str()));
  }

  template <typename T>
  void StructuredBuffer<T>::BindSRVGraphic(const eCommandList list)
  {
    if (m_b_uav_bound_)
    {
      throw std::logic_error("StructuredBuffer is bound as UAV, cannot bind as SRV");
    }

    if (m_b_srv_bound_) { return; }

    const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
      );

    GetD3Device().GetCommandList(list)->ResourceBarrier(1, &srv_transition);

    if constexpr (is_client_sb<T>::value == true)
    {
      GetRenderPipeline().GetDescriptor().SetShaderResource
        (
         m_srv_heap_->GetCPUDescriptorHandleForHeapStart(),
         which_client_sb<T>::value
        );
    }
    else if constexpr (is_sb<T>::value == true)
    {
      GetRenderPipeline().GetDescriptor().SetShaderResource
        (
         m_srv_heap_->GetCPUDescriptorHandleForHeapStart(),
         which_sb<T>::value
        );
    }

    m_b_srv_bound_ = true;
  }

  template <typename T>
  void StructuredBuffer<T>::UnbindSRVGraphic(const eCommandList list)
  {
    if (!m_b_srv_bound_) { return; }

    const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
       D3D12_RESOURCE_STATE_COMMON
      );

    GetD3Device().GetCommandList(list)->ResourceBarrier(1, &srv_transition);

    m_b_srv_bound_ = false;
  }

  template <typename T>
  void StructuredBuffer<T>::Create(const eCommandList list, UINT size, const T* initial_data, bool is_mutable)
  {
    if (size == 0)
    {
      size = 1;
    }

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
      SetData(list, size, data.data());
    }
    InitializeReadBuffer(size);
  }

  template <typename T>
  void StructuredBuffer<T>::SetData(const eCommandList list, const UINT size, const T* src_ptr)
  {
    if (!m_b_mutable_) { throw std::logic_error("StructuredBuffer is defined as not mutable"); }

    if (m_size_ < size) { Create(list, size, nullptr, m_b_mutable_); }

    ComPtr<ID3D12Resource> upload_buffer;

    const std::string gen_type_name = typeid(T).name();
    const std::wstring type_name(gen_type_name.begin(), gen_type_name.end());
    const std::wstring buffer_name = L"StructuredBuffer Write " + type_name;

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommittedResource
       (
        &upload_heap,
        D3D12_HEAP_FLAG_NONE,
        &buffer_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(upload_buffer.GetAddressOf())
       )
      );

    DX::ThrowIfFailed(upload_buffer->SetName(buffer_name.c_str()));

    char* data = nullptr;

    DX::ThrowIfFailed(upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&data)));

    std::memcpy(data, src_ptr, sizeof(T) * size);

    upload_buffer->Unmap(0, nullptr);

    const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition
    (
     m_buffer_.Get(),
     D3D12_RESOURCE_STATE_COMMON,
     D3D12_RESOURCE_STATE_COPY_DEST
    );
  
    GetD3Device().GetCommandList(list)->ResourceBarrier(1, &barrier);

    GetD3Device().GetCommandList(list)->CopyResource
      (
       m_buffer_.Get(),
       0,
       upload_buffer.Get(),
       0,
       sizeof(T) * size
      );

    const auto& revert_barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COPY_DEST,
       D3D12_RESOURCE_STATE_COMMON
      );

    GetD3Device().GetCommandList(list)->ResourceBarrier(1, &revert_barrier);
    
    GetGC().Track(upload_buffer);
  }

  // This will execute the command list and copy the data from the GPU to the CPU.
  template <typename T>
  void StructuredBuffer<T>::GetData(const UINT size, T* dst_ptr)
  {
    // For now, common state is the only state that can be used for copying.
    if (m_b_srv_bound_ || m_b_uav_bound_)
    {
      throw std::logic_error("StructuredBuffer is bound as SRV or UAV, cannot get data");
    }

    GetD3Device().WaitAndReset(COMMAND_LIST_COPY);

    const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_COPY_SOURCE
      );

    GetD3Device().GetCommandList(COMMAND_LIST_COPY)->ResourceBarrier(1, &barrier);

    GetD3Device().GetCommandList(COMMAND_LIST_COPY)->CopyResource
      (
       m_read_buffer_.Get(),
       m_buffer_.Get()
      );

    const auto& revert_barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COPY_SOURCE,
       D3D12_RESOURCE_STATE_COMMON
      );

    GetD3Device().GetCommandList(COMMAND_LIST_COPY)->ResourceBarrier(1, &revert_barrier);

    GetD3Device().ExecuteCommandList(COMMAND_LIST_COPY);

    // Wait until the copy is done.
    GetD3Device().Wait(COMMAND_LIST_COPY);

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
}

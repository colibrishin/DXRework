#pragma once
#include <BufferHelpers.h>
#include <d3dx12.h>

#include "Windows.h"
#include "egCommands.h"
#include "egCommon.hpp"
#include "egDescriptors.h"
#include "egType.h"

namespace Engine::Graphics
{
  class StructuredBufferBase
  {
  public:
    virtual ~StructuredBufferBase() = default;

    virtual void TransitionToSRV(ID3D12GraphicsCommandList1* cmd) const = 0;
    virtual void TransitionToUAV(ID3D12GraphicsCommandList1* cmd) const = 0;
    virtual void TransitionCommon(ID3D12GraphicsCommandList1 * cmd, const D3D12_RESOURCE_STATES before) const = 0;

    virtual void CopySRVHeap(const DescriptorPtr& heap) const = 0;
    virtual void CopyUAVHeap(const DescriptorPtr& heap) const = 0;

  protected:
    ComPtr<ID3D12DescriptorHeap> m_srv_heap_;
    ComPtr<ID3D12DescriptorHeap> m_uav_heap_;

    ComPtr<ID3D12Resource> m_upload_buffer_;
    ComPtr<ID3D12Resource> m_read_buffer_;
    ComPtr<ID3D12Resource> m_buffer_;

  };

  template <typename T>
  class StructuredBuffer : public StructuredBufferBase
  {
  public:
    StructuredBuffer();
    ~StructuredBuffer() = default;

    void            Create(ID3D12GraphicsCommandList1 * cmd, UINT size, const T * initial_data);
    void __fastcall SetData(ID3D12GraphicsCommandList1 * cmd, const UINT size, const T* src_ptr);
    void __fastcall GetData(UINT size, T* dst_ptr);
    void            Clear();

    void TransitionToSRV(ID3D12GraphicsCommandList1* cmd) const override;
    void TransitionToUAV(ID3D12GraphicsCommandList1* cmd) const override;
    void TransitionCommon(ID3D12GraphicsCommandList1 * cmd, const D3D12_RESOURCE_STATES before) const override;

    void CopySRVHeap(const DescriptorPtr& heap) const override;
    void CopyUAVHeap(const DescriptorPtr& heap) const override;
    D3D12_GPU_VIRTUAL_ADDRESS  GetGPUAddress() const;

  private:
    void InitializeSRV(UINT size);
    void InitializeUAV(UINT size);
    void InitializeMainBuffer(ID3D12GraphicsCommandList1* cmd, UINT size);
    void InitializeUploadBuffer(ID3D12GraphicsCommandList1* cmd, UINT size, const T* initial_data);
    void InitializeReadBuffer(UINT size);

    UINT   m_size_;
    UINT   m_aligned_t_size_;
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
        .StructureByteStride = m_aligned_t_size_,
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
    uav_desc.Buffer.StructureByteStride = m_aligned_t_size_;
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
  void StructuredBuffer<T>::InitializeMainBuffer(ID3D12GraphicsCommandList1* cmd, UINT size)
  {
    const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(m_aligned_t_size_ * size);

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
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &buffer_desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_buffer_.ReleaseAndGetAddressOf())
       )
      );

    DX::ThrowIfFailed(m_buffer_->SetName(buffer_name.c_str()));
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeUploadBuffer(ID3D12GraphicsCommandList1* cmd, UINT size, const T* initial_data)
  {
    const auto& upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(m_aligned_t_size_ * size);

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommittedResource
       (
        &upload_heap,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &buffer_desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_upload_buffer_.ReleaseAndGetAddressOf())
       )
      );

    const std::string  gen_type_name = typeid(T).name();
    const std::wstring type_name(gen_type_name.begin(), gen_type_name.end());
    const std::wstring buffer_name = L"StructuredBuffer " + type_name;

    const std::wstring write_buffer_name = L"StructuredBuffer Write " + type_name;

    DX::ThrowIfFailed(m_upload_buffer_->SetName(write_buffer_name.c_str()));

    if (initial_data != nullptr)
    {
      char* data = nullptr;

      DX::ThrowIfFailed(m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));

      _mm256_memcpy(data, initial_data, sizeof(T) * size);

      m_upload_buffer_->Unmap(0, nullptr);

      cmd->CopyResource
        (
         m_buffer_.Get(),
         m_upload_buffer_.Get()
        );

      const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_COMMON
        );

      cmd->ResourceBarrier(1, &common_transition);
    }
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeReadBuffer(UINT size)
  {
    const auto& readback_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(m_aligned_t_size_ * size);

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommittedResource
       (
        &readback_heap,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &buffer_desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_read_buffer_.ReleaseAndGetAddressOf())
       )
      );
  }

  template <typename T>
  StructuredBuffer<T>::StructuredBuffer()
    : m_size_(0)
  {
    static_assert(sizeof(T) <= 2048, "StructuredBuffer struct T size is too big");
    m_aligned_t_size_ = Engine::Align(sizeof(T), 16);
  }

  template <typename T>
  void StructuredBuffer<T>::Create(ID3D12GraphicsCommandList1* cmd, UINT size, const T* initial_data)
  {
    if (size == 0)
    {
      size = 1;
    }

    m_size_ = size;

    InitializeMainBuffer(cmd, size);
    InitializeSRV(size);
    if constexpr (is_uav_sb<T>::value == true)
    {
      InitializeUAV(size);
    }
    InitializeUploadBuffer(cmd, size, initial_data);
    InitializeReadBuffer(size);
  }

  template <typename T>
  void StructuredBuffer<T>::SetData(ID3D12GraphicsCommandList1* cmd, const UINT size, const T* src_ptr)
  {
    if (m_size_ < size)
    {
      Create(cmd, size, src_ptr);
      return;
    }

    char* data = nullptr;

    DX::ThrowIfFailed(m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));

    _mm256_memcpy(data, src_ptr, sizeof(T) * size);

    m_upload_buffer_->Unmap(0, nullptr);

    cmd->CopyResource(m_buffer_.Get(), m_upload_buffer_.Get());

    const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COPY_DEST,
       D3D12_RESOURCE_STATE_COMMON
      );

    cmd->ResourceBarrier(1, &common_transition);
  }

  // This will execute the command list and copy the data from the GPU to the CPU.
  template <typename T>
  void StructuredBuffer<T>::GetData(const UINT size, T* dst_ptr)
  {
    GetD3Device().WaitAndReset(COMMAND_LIST_UPDATE);

    const auto& cmd = GetD3Device().GetCommandList(COMMAND_LIST_UPDATE);

    const auto& copy_barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_COPY_SOURCE
      );

    const auto& revert_barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COPY_SOURCE,
       D3D12_RESOURCE_STATE_COMMON
      );

    cmd->ResourceBarrier(1, &copy_barrier);
    cmd->CopyResource
      (
       m_read_buffer_.Get(),
       m_buffer_.Get()
      );
    cmd->ResourceBarrier(1, &revert_barrier);

    DX::ThrowIfFailed(cmd->Close());

    ID3D12CommandList* lists[] = { cmd };

    GetD3Device().GetCommandQueue(COMMAND_LIST_UPDATE)->ExecuteCommandLists(1, lists);

    GetD3Device().Wait();

    char* data = nullptr;

    DX::ThrowIfFailed(m_read_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));

    _mm256_memcpy(dst_ptr, data, sizeof(T) * size);

    m_read_buffer_->Unmap(0, nullptr);
  }

  template <typename T>
  void StructuredBuffer<T>::Clear()
  {
    m_buffer_.Reset();
  }

  template <typename T>
  void StructuredBuffer<T>::TransitionToSRV(ID3D12GraphicsCommandList1* cmd) const
  {
    const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
      );

    cmd->ResourceBarrier(1, &srv_transition);
  }

  template <typename T>
  void StructuredBuffer<T>::TransitionToUAV(ID3D12GraphicsCommandList1* cmd) const
  {
    const auto& uav_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_UNORDERED_ACCESS
      );

    cmd->ResourceBarrier(1, &uav_transition);
  }

  template <typename T>
  void StructuredBuffer<T>::TransitionCommon(ID3D12GraphicsCommandList1* cmd, const D3D12_RESOURCE_STATES before) const
  {
    const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       before,
       D3D12_RESOURCE_STATE_COMMON
      );

    cmd->ResourceBarrier(1, &common_transition);
  }

  template <typename T>
  void StructuredBuffer<T>::CopySRVHeap(const DescriptorPtr& w_heap) const
  {
    const auto& heap = w_heap.lock();

    if constexpr (is_client_sb<T>::value == true)
    {
      heap->SetShaderResource
        (
         m_srv_heap_->GetCPUDescriptorHandleForHeapStart(),
         which_client_sb<T>::value
        );
    }
    else if constexpr (is_sb<T>::value == true)
    {
      heap->SetShaderResource
        (
         m_srv_heap_->GetCPUDescriptorHandleForHeapStart(),
         which_sb<T>::value
        );
    }
  }

  template <typename T>
  void StructuredBuffer<T>::CopyUAVHeap(const DescriptorPtr& w_heap) const
  {
    const auto& heap = w_heap.lock();

    if constexpr (is_client_uav_sb<T>::value == true)
    {
      heap->SetUnorderedAccess
        (
         m_uav_heap_->GetCPUDescriptorHandleForHeapStart(),
         which_client_sb_uav<T>::value
        );
    }
    else if constexpr (is_uav_sb<T>::value == true)
    {
      heap->SetUnorderedAccess
        (
         m_uav_heap_->GetCPUDescriptorHandleForHeapStart(),
         which_sb_uav<T>::value
        );
    }
  }

  template <typename T>
  D3D12_GPU_VIRTUAL_ADDRESS StructuredBuffer<T>::GetGPUAddress() const
  {
    return m_buffer_->GetGPUVirtualAddress();
  }
}

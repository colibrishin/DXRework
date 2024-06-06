#pragma once
#include <BufferHelpers.h>
#include <d3dx12.h>

#include "Windows.h"
#include "egCommands.h"
#include "egCommon.hpp"
#include "egDescriptors.h"
#include "egGarbageCollector.h"
#include "egType.h"

namespace Engine::Graphics
{
  class StructuredBufferBase
  {
  public:
    virtual ~StructuredBufferBase() = default;

    virtual void BindSRVGraphic(const CommandPair& cmd, const DescriptorPtr& heap) = 0;
    virtual void UnbindSRVGraphic(const CommandPair& cmd) const = 0;

    virtual void BindUAVGraphic(const CommandPair & cmd, const DescriptorPtr & heap) = 0;
    virtual void UnbindUAVGraphic(const CommandPair& cmd) const = 0;

    virtual void BindSRVGraphic(ID3D12GraphicsCommandList* cmd, const DescriptorPtr& heap) = 0;
    virtual void UnbindSRVGraphic(ID3D12GraphicsCommandList* cmd) const = 0;

    virtual void BindUAVGraphic(ID3D12GraphicsCommandList* cmd, const DescriptorPtr& heap) = 0;
    virtual void UnbindUAVGraphic(ID3D12GraphicsCommandList* cmd) const = 0;

  protected:
    ComPtr<ID3D12DescriptorHeap> m_srv_heap_;
    ComPtr<ID3D12DescriptorHeap> m_uav_heap_;

    ComPtr<ID3D12Resource> m_buffer_;

  };

  template <typename T>
  class StructuredBuffer : public StructuredBufferBase
  {
  public:
    StructuredBuffer();
    ~StructuredBuffer() = default;

    void            Create(UINT size, const T * initial_data);
    void __fastcall SetData(const UINT size, const T * src_ptr);
    void __fastcall GetData(UINT size, T* dst_ptr);
    void            Clear();

    void BindSRVGraphic(const CommandPair& cmd, const DescriptorPtr& heap) override;
    void UnbindSRVGraphic(const CommandPair& cmd) const override;

    void BindSRVGraphic(ID3D12GraphicsCommandList* cmd, const DescriptorPtr& heap) override;
    void UnbindSRVGraphic(ID3D12GraphicsCommandList* cmd) const override;

    void BindUAVGraphic(const CommandPair & cmd, const DescriptorPtr & heap) override;
    void UnbindUAVGraphic(const CommandPair& cmd) const override;

    void BindUAVGraphic(ID3D12GraphicsCommandList* cmd, const DescriptorPtr& heap) override;
    void UnbindUAVGraphic(ID3D12GraphicsCommandList* cmd) const override;

  private:
    void InitializeSRV(UINT size);
    void InitializeUAV(UINT size);
    void InitializeMainBuffer(UINT size, const T * initial_data);

    std::vector<T> m_data_;
    UINT m_size_;

    bool m_b_dirty_;
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

    const auto& cmd = GetD3Device().GetCommandList(COMMAND_LIST_COPY);

    GetD3Device().WaitAndReset(COMMAND_LIST_COPY);

    if (initial_data != nullptr)
    {
      ComPtr<ID3D12Resource> upload_buffer;

      DX::ThrowIfFailed
        (
         DirectX::CreateUploadBuffer
         (
          GetD3Device().GetDevice(),
          initial_data,
          size,
          upload_buffer.ReleaseAndGetAddressOf()
         )
        );

      const std::wstring write_buffer_name = L"StructuredBuffer Write " + type_name;

      DX::ThrowIfFailed(upload_buffer->SetName(write_buffer_name.c_str()));

      char* data = nullptr;

      DX::ThrowIfFailed(upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&data)));

      std::memcpy(data, initial_data, sizeof(T) * size);

      upload_buffer->Unmap(0, nullptr);

      cmd->CopyResource
        (
         m_buffer_.Get(),
         upload_buffer.Get()
        );

      const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_COMMON
        );

      cmd->ResourceBarrier(1, &common_transition);

      GetGC().Track(upload_buffer);
    }

    GetD3Device().ExecuteCommandList(COMMAND_LIST_COPY);

    m_b_dirty_ = false;
  }

  template <typename T>
  StructuredBuffer<T>::StructuredBuffer()
    : m_data_(),
      m_size_(0),
      m_b_dirty_(false)
  {
    static_assert(sizeof(T) <= 2048, "StructuredBuffer struct T size is too big");
    static_assert(sizeof(T) % sizeof(Vector4) == 0, "StructuredBuffer struct T size need to be dividable by 16");
  }

  template <typename T>
  void StructuredBuffer<T>::BindSRVGraphic(const CommandPair& cmd, const DescriptorPtr& heap)
  {
    BindSRVGraphic(cmd.GetList(), heap);
  }

  template <typename T>
  void StructuredBuffer<T>::UnbindSRVGraphic(const CommandPair& cmd) const
  {
    UnbindSRVGraphic(cmd.GetList());
  }

  template <typename T>
  void StructuredBuffer<T>::BindSRVGraphic(ID3D12GraphicsCommandList* cmd, const DescriptorPtr& heap)
  {
    if (m_b_dirty_)
    {
      ComPtr<ID3D12Resource> upload_buffer;

      DX::ThrowIfFailed
        (
         DirectX::CreateUploadBuffer
         (
          GetD3Device().GetDevice(),
          m_data_.data(),
          m_data_.size(),
          upload_buffer.ReleaseAndGetAddressOf()
         )
        );

      char* data = nullptr;

      DX::ThrowIfFailed(upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&data)));

      std::memcpy(data, m_data_.data(), sizeof(T) * m_data_.size());

      upload_buffer->Unmap(0, nullptr);

      cmd->CopyResource(m_buffer_.Get(), upload_buffer.Get());

      const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_COMMON
        );

      cmd->ResourceBarrier(1, &common_transition);

      GetGC().Track(upload_buffer);

      m_b_dirty_ = false;
    }

    const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
      );

    cmd->ResourceBarrier(1, &srv_transition);

    if constexpr (is_client_sb<T>::value == true)
    {
      heap.SetShaderResource
        (
         m_srv_heap_->GetCPUDescriptorHandleForHeapStart(),
         which_client_sb<T>::value
        );
    }
    else if constexpr (is_sb<T>::value == true)
    {
      heap.SetShaderResource
        (
         m_srv_heap_->GetCPUDescriptorHandleForHeapStart(),
         which_sb<T>::value
        );
    }
  }

  template <typename T>
  void StructuredBuffer<T>::UnbindSRVGraphic(ID3D12GraphicsCommandList* cmd) const
  {
    const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
       D3D12_RESOURCE_STATE_COMMON
      );

    cmd->ResourceBarrier(1, &srv_transition);
  }

  template <typename T>
  void StructuredBuffer<T>::BindUAVGraphic(ID3D12GraphicsCommandList* cmd, const DescriptorPtr& heap)
  {
    if (m_b_dirty_)
    {
      ComPtr<ID3D12Resource> upload_buffer;

      DX::ThrowIfFailed
        (
         DirectX::CreateUploadBuffer
         (
          GetD3Device().GetDevice(),
          m_data_.data(),
          m_data_.size(),
          upload_buffer.ReleaseAndGetAddressOf()
         )
        );

      char* data = nullptr;

      DX::ThrowIfFailed(upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&data)));

      std::memcpy(data, m_data_.data(), sizeof(T) * m_data_.size());

      upload_buffer->Unmap(0, nullptr);


      cmd->CopyResource(m_buffer_.Get(), upload_buffer.Get());

      const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_COMMON
        );

      cmd->ResourceBarrier(1, &common_transition);

      GetGC().Track(upload_buffer);

      m_b_dirty_ = false;
    }

    const auto& uav_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_UNORDERED_ACCESS
      );

    cmd->ResourceBarrier(1, &uav_transition);

    if constexpr (is_client_uav_sb<T>::value == true)
    {
      heap.SetUnorderedAccess
        (
         m_uav_heap_->GetCPUDescriptorHandleForHeapStart(),
         which_client_sb_uav<T>::value
        );
    }
    else if constexpr (is_uav_sb<T>::value == true)
    {
      heap.SetUnorderedAccess
        (
         m_uav_heap_->GetCPUDescriptorHandleForHeapStart(),
         which_sb_uav<T>::value
        );
    }
  }

  template <typename T>
  void StructuredBuffer<T>::UnbindUAVGraphic(ID3D12GraphicsCommandList* cmd) const
  {
    const auto& uav_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
            m_buffer_.Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_COMMON
           );

    cmd->ResourceBarrier(1, &uav_transition);
  }

  template <typename T>
  void StructuredBuffer<T>::BindUAVGraphic(const CommandPair& cmd, const DescriptorPtr& heap)
  {
    BindUAVGraphic(cmd.GetList(), heap);
  }

  template <typename T>
  void StructuredBuffer<T>::UnbindUAVGraphic(const CommandPair& cmd) const
  {
    UnbindUAVGraphic(cmd.GetList());
  }

  template <typename T>
  void StructuredBuffer<T>::Create(UINT size, const T* initial_data)
  {
    if (size == 0)
    {
      size = 1;
    }

    m_size_      = size;
    m_data_.resize(size);

    if (initial_data != nullptr)
    {
      std::memcpy(m_data_.data(), initial_data, sizeof(T) * size);
    }

    InitializeMainBuffer(size, initial_data);
    InitializeSRV(size);
    if constexpr (is_uav_sb<T>::value == true)
    {
      InitializeUAV(size);
    }
  }

  template <typename T>
  void StructuredBuffer<T>::SetData(const UINT size, const T* src_ptr)
  {
    if (m_size_ < size) { Create(size, nullptr); }

    if (src_ptr != nullptr)
    {
      std::memcpy(m_data_.data(), src_ptr, sizeof(T) * size);
    }
    else
    {
      std::memset(m_data_.data(), 0, sizeof(T) * size);
    }

    m_b_dirty_ = true;
  }

  // This will execute the command list and copy the data from the GPU to the CPU.
  template <typename T>
  void StructuredBuffer<T>::GetData(const UINT size, T* dst_ptr)
  {
    GetD3Device().WaitAndReset(COMMAND_LIST_COPY);

    const auto& cmd = GetD3Device().GetCommandList(COMMAND_LIST_COPY);

    const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_COPY_SOURCE
      );

    ComPtr<ID3D12Resource> read_buffer_;

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
        IID_PPV_ARGS(read_buffer_.ReleaseAndGetAddressOf())
       )
      );

    cmd->ResourceBarrier(1, &barrier);

    cmd->CopyResource
      (
       read_buffer_.Get(),
       m_buffer_.Get()
      );

    const auto& revert_barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COPY_SOURCE,
       D3D12_RESOURCE_STATE_COMMON
      );

    cmd->ResourceBarrier(1, &revert_barrier);

    GetD3Device().ExecuteCommandList(COMMAND_LIST_COPY);

    // Wait until the copy is done.
    GetD3Device().Wait(COMMAND_LIST_COPY);

    char* data = nullptr;

    DX::ThrowIfFailed(read_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));

    std::memcpy(dst_ptr, data, sizeof(T) * size);

    read_buffer_->Unmap(0, nullptr);
  }

  template <typename T>
  void StructuredBuffer<T>::Clear()
  {
    m_buffer_.Reset();
  }
}

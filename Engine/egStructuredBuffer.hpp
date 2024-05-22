#pragma once
#include <BufferHelpers.h>
#include <d3dx12.h>

#include "Windows.h"
#include "egCommon.hpp"
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
    void            BindSRV();
    void            UnbindSRV();
    void            Clear();

    void BindSRVGraphicDeferred();
    void UnbindSRVGraphicDeferred();
    void BindSRVComputeDeferred();
    void UnbindSRVComputeDeferred();

    template <typename U = T, typename std::enable_if_t<is_uav_sb<U>::value, bool> = true>
    void BindUAVGraphicDeferred()
    {
      if (m_b_srv_bound_ || m_b_srv_bound_compute_)
      {
        throw std::logic_error("StructuredBuffer is bound as SRV, cannot bind as UAV");
      }

      if (!m_b_uav_bound_ || m_b_uav_bound_compute_) { return; }

      {
        CommandGuard cg;
        const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition
                (
                 m_buffer_.Get(),
                 D3D12_RESOURCE_STATE_COMMON,
                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                );
      
        if constexpr (is_client_uav_sb<T>::value == true)
        {
          GetD3Device().GetCommandList()->SetComputeRootUnorderedAccessView
            (
             which_client_sb_uav<T>::value,
             m_uav_description_heap_->GetCPUDescriptorHandleForHeapStart()
            );

        }
        if constexpr (!is_client_uav_sb<T>::value && is_uav_sb<T>::value)
        {
          GetD3Device().GetCommandList()->SetComputeRootUnorderedAccessView
            (
             which_sb_uav<T>::value,
             m_uav_description_heap_->GetCPUDescriptorHandleForHeapStart()
            );
        }

        GetD3Device().GetCommandList()->ResourceBarrier(1, &barrier);
      }
      
      m_b_uav_bound_ = true;
    }

    template <typename U = T, typename std::enable_if_t<is_uav_sb<U>::value, bool> = true>
    void BindUAVComputeDeferred()
    {
      if (m_b_srv_bound_ || m_b_srv_bound_compute_)
      {
        throw std::logic_error("StructuredBuffer is bound as SRV, cannot bind as UAV");
      }

      {
        CommandGuard cg;

        const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
         D3D12_RESOURCE_STATE_COMMON
        );

        if constexpr (is_client_uav_sb<T>::value == true)
        {
          GetD3Device().GetCommandList()->SetComputeRootUnorderedAccessView
            (
             which_client_sb_uav<T>::value,
              0
            );

        }
        if constexpr (!is_client_uav_sb<T>::value && is_uav_sb<T>::value)
        {
          GetD3Device().GetCommandList()->SetComputeRootUnorderedAccessView
            (
             which_sb_uav<T>::value,
             0
            );
        }

        GetD3Device().GetCommandList()->ResourceBarrier(1, &barrier);
      }

      GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &uav_transition);

      m_b_uav_bound_ = false;
    }

    template <typename U = T, typename std::enable_if_t<is_uav_sb<U>::value, bool> = true>
    void UnbindUAVComputeDeferred()
    {
      if (!m_b_uav_bound_compute_) { return; }

      const auto& uav_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
         D3D12_RESOURCE_STATE_COMMON
        );

      GetD3Device().GetComputeCommandList()->ResourceBarrier(1, &uav_transition);

      m_b_uav_bound_compute_ = false;
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
    
    ComPtr<ID3D12DescriptorHeap> m_srv_description_heap_;
    ComPtr<ID3D12DescriptorHeap> m_uav_description_heap_;

    ComPtr<ID3D12Resource> m_write_buffer_;
    ComPtr<ID3D12Resource> m_read_buffer_;
  };

  template <typename T>
  void StructuredBuffer<T>::InitializeSRV(const UINT size)
  {
    constexpr D3D12_DESCRIPTOR_HEAP_DESC heap_desc
    {
      .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 1,
      .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask       = 0
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &heap_desc,
        IID_PPV_ARGS(m_srv_description_heap_.ReleaseAndGetAddressOf())
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
       m_srv_description_heap_->GetCPUDescriptorHandleForHeapStart()
     );
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeUAV(UINT size)
  {
    constexpr D3D12_DESCRIPTOR_HEAP_DESC heap_desc
    {
      .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 1,
      .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask       = 0
    };
    
    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &heap_desc,
        IID_PPV_ARGS(m_uav_description_heap_.ReleaseAndGetAddressOf())
       )
      );
    
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
    uav_desc.Format              = DXGI_FORMAT_UNKNOWN;
    uav_desc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements  = size;

    GetD3Device().GetDevice()->CreateUnorderedAccessView
     (
       m_buffer_.Get(),
       nullptr,
       &uav_desc,
       m_uav_description_heap_->GetCPUDescriptorHandleForHeapStart()
     );
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeMainBuffer(UINT size, const T* initial_data)
  {
    if ((sizeof(T) * size) % 16 != 0)
    {
      throw std::runtime_error("StructuredBuffer size need to be dividable by 16");
    }
    
    if (initial_data != nullptr)
    {
      const auto& end_state = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_COMMON
        );
      
      GetD3Device().CreateBuffer1D<T>
      (
        m_buffer_.GetAddressOf(),
        initial_data,
        size,
        end_state
      );
    }
    else
    {
      GetD3Device().CreateBuffer<T>(m_buffer_.GetAddressOf(), size);
    }
  }

  template <typename T>
  void StructuredBuffer<T>::InitializeWriteBuffer(const UINT size)
  {
    GetD3Device().CreateBuffer<T>(
      m_write_buffer_.GetAddressOf(),
      size,
      D3D12_HEAP_TYPE_UPLOAD,
      D3D12_RESOURCE_STATE_GENERIC_READ);
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
    GetD3Device().CreateBuffer<T>(
      m_read_buffer_.ReleaseAndGetAddressOf(),
      size,
      D3D12_HEAP_TYPE_READBACK,
      D3D12_RESOURCE_STATE_COPY_DEST);
  }

  template <typename T>
  void StructuredBuffer<T>::BindSRVGraphicDeferred()
  {
    if (m_b_uav_bound_)
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

    GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &srv_transition);

    m_b_srv_bound_ = true;
  }

  template <typename T>
  void StructuredBuffer<T>::UnbindSRVGraphicDeferred()
  {
    if (!m_b_srv_bound_ || m_b_srv_bound_compute_) { return; }

    const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
       D3D12_RESOURCE_STATE_COMMON
      );

    GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &srv_transition);

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
      SetData(size, data.data());
    }
    InitializeReadBuffer(size);
  }

  template <typename T>
  void StructuredBuffer<T>::SetData(const UINT size, const T* src_ptr)
  {
    if (!m_b_mutable_) { throw std::logic_error("StructuredBuffer is defined as not mutable"); }

    if (m_size_ < size) { Create(size, nullptr, m_b_mutable_); }

    {
      ForceCommandExecutionGuard fcg;
      const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_COPY_DEST
      );
    
      GetD3Device().GetCommandList()->ResourceBarrier(1, &barrier);

      const D3D12_SUBRESOURCE_DATA data_desc
      {
        .pData = src_ptr,
        .RowPitch = sizeof(T) * size,
        .SlicePitch = sizeof(T) * size
      };

      UpdateSubresources
      (
        GetD3Device().GetCommandList(),
        m_buffer_.Get(),
        m_write_buffer_.Get(),
        0,
        0,
        1,
        &data_desc
      );

      const auto& revert_barrier = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_COMMON
        );
    
      GetD3Device().GetCommandList()->ResourceBarrier(1, &revert_barrier);
    }
  }

  template <typename T>
  void StructuredBuffer<T>::GetData(UINT size, T* dst_ptr)
  {
    {
      ForceCommandExecutionGuard fcg;
      GetD3Device().GetCommandList()->CopyResource(m_read_buffer_.Get(), m_buffer_.Get());
    }
    constexpr D3D12_RANGE range = { 0, 0 };

    UINT8* map_ptr = nullptr;
    DX::ThrowIfFailed(m_buffer_->Map(0, &range, reinterpret_cast<void**>(&map_ptr)));
    std::memcpy(dst_ptr, map_ptr, sizeof(T) * size);
    m_buffer_->Unmap(0, nullptr);
  }

  template <typename T>
  void StructuredBuffer<T>::BindSRV()
  {
    if (m_b_uav_bound_) { throw std::logic_error("StructuredBuffer is already bound as UAV"); }

    {
      CommandGuard cg;
      
      const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
      );

      GetD3Device().GetCommandList()->ResourceBarrier(1, &barrier);

      if constexpr (is_client_sb<T>::value == true)
      {
        GetD3Device().GetCommandList()->SetGraphicsRootShaderResourceView
          (which_client_sb<T>::value, m_buffer_->GetGPUVirtualAddress());
      }
      else
      {
        GetD3Device().GetCommandList()->SetGraphicsRootShaderResourceView
          (which_sb<T>::value, m_buffer_->GetGPUVirtualAddress());
      }
    }
    
    m_b_srv_bound_ = true;
  }

  // This will execute the command list and copy the data from the GPU to the CPU.
  template <typename T>
  void StructuredBuffer<T>::UnbindSRV()
  {
    {
      CommandGuard cg;

      const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_buffer_.Get(),
       D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
       D3D12_RESOURCE_STATE_COMMON
      );

      GetD3Device().GetCommandList()->ResourceBarrier(1, &barrier);
    
      if constexpr (is_client_sb<T>::value == true)
      {
        GetD3Device().GetCommandList()->SetGraphicsRootShaderResourceView(
          which_client_sb<T>::value, 0);
      }
      else
      {
        GetD3Device().GetCommandList()->SetGraphicsRootShaderResourceView(
          which_sb<T>::value, 0);
      }
    }
    
    m_b_srv_bound_ = false;
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

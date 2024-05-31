#pragma once
#include "egGlobal.h"
#include "egRenderPipeline.h"

namespace Engine::Graphics
{
  inline static std::vector<ComPtr<ID3D12Resource>> g_cb_upload_buffers = {};

  static void _reset_constant_buffer()
  {
    for (auto& buffer : g_cb_upload_buffers)
    {
      buffer.Reset();
    }

    g_cb_upload_buffers.clear();
  }

  // Creates a constant buffer for only current back buffer.
  template <typename T>
  class ConstantBuffer
  {
  public:
    ConstantBuffer();

    void Create(const T* src_data)
    {
      DirectCommandGuard dcg;

      const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
      const auto& cb_desc      = CD3DX12_RESOURCE_DESC::Buffer(m_alignment_size_);

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &default_heap,
          D3D12_HEAP_FLAG_NONE,
          &cb_desc,
          D3D12_RESOURCE_STATE_COPY_DEST,
          nullptr,
          IID_PPV_ARGS(m_buffer_.GetAddressOf())
         )
        );

      if (src_data != nullptr)
      {
        ComPtr<ID3D12Resource> upload_buffer;

        DX::ThrowIfFailed
          (
           DirectX::CreateUploadBuffer
           (
            GetD3Device().GetDevice(),
            src_data,
            1,
            upload_buffer.GetAddressOf()
           )
          );

        g_cb_upload_buffers.push_back(upload_buffer);

        char* data = nullptr;

        DX::ThrowIfFailed(upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&data)));
        std::memcpy(data, src_data, sizeof(T));
        upload_buffer->Unmap(0, nullptr);

        GetD3Device().GetCommandList()->CopyResource(m_buffer_.Get(), upload_buffer.Get());
      }

      const auto& cb_trans = CD3DX12_RESOURCE_BARRIER::Transition
        (m_buffer_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

      GetD3Device().GetCommandList()->ResourceBarrier(1, &cb_trans);

      const D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc
      {
        .BufferLocation = m_buffer_->GetGPUVirtualAddress(),
        .SizeInBytes    = m_alignment_size_
      };

      GetD3Device().CreateConstantBufferView(which_cb<T>::value, cbv_desc);
    }

    void SetData(const T* src_data)
    {
      DirectCommandGuard dcg;

      const auto& copy_trans = CD3DX12_RESOURCE_BARRIER::Transition(m_buffer_.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);

      GetD3Device().GetCommandList()->ResourceBarrier(1, &copy_trans);

      // Use upload buffer for synchronization.
      ComPtr<ID3D12Resource> upload_buffer;

      DX::ThrowIfFailed
        (
         DirectX::CreateUploadBuffer
         (
          GetD3Device().GetDevice(),
          src_data,
          1,
          upload_buffer.GetAddressOf()
         )
        );

      g_cb_upload_buffers.push_back(upload_buffer);

      char* data = nullptr;

      DX::ThrowIfFailed(upload_buffer->Map(0, nullptr, reinterpret_cast<void**
      >(&data)));

      std::memcpy(data, src_data, sizeof(T));

      upload_buffer->Unmap(0, nullptr);

      GetD3Device().GetCommandList()->CopyResource(m_buffer_.Get(), upload_buffer.Get());

      const auto& cb_trans = CD3DX12_RESOURCE_BARRIER::Transition(m_buffer_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

      GetD3Device().GetCommandList()->ResourceBarrier(1, &cb_trans);
    }

  private:

    ComPtr<ID3D12Resource> m_buffer_;
    UINT m_alignment_size_;

  };

  template <typename T>
  ConstantBuffer<T>::ConstantBuffer()
  {
    static_assert(std::is_pod_v<T>, "Constant buffer type must be a POD type");

    m_alignment_size_ = (sizeof(T) + 255) & ~255; 
  }
}
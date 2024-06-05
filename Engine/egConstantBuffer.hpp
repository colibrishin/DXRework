#pragma once
#include <d3dx12.h>

#include "egGarbageCollector.h"
#include "egGlobal.h"
#include "egRenderPipeline.h"
#include "egRenderPipeline.h"

namespace Engine::Graphics
{
  // Creates a constant buffer for only current back buffer.
  template <typename T>
  class ConstantBuffer
  {
  public:
    ConstantBuffer();

    void Create(const T* src_data)
    {
      GetD3Device().WaitAndReset(COMMAND_LIST_UPDATE);

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

      const auto         gen_type_name = std::string(typeid(T).name());
      const auto         type_name     = std::wstring(gen_type_name.begin(), gen_type_name.end());
      const std::wstring buffer_name   = type_name + L" Constant Buffer";

      DX::ThrowIfFailed(m_buffer_->SetName(buffer_name.c_str()));

      if (src_data != nullptr)
      {
        ComPtr<ID3D12Resource> upload_buffer;

        const auto& upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(m_alignment_size_);

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

        const std::wstring upload_buffer_name = type_name + L" Constant Buffer Upload Buffer";

        DX::ThrowIfFailed(upload_buffer->SetName(upload_buffer_name.c_str()));

        char* data = nullptr;

        DX::ThrowIfFailed(upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&data)));
        std::memcpy(data, src_data, sizeof(T));
        upload_buffer->Unmap(0, nullptr);

        GetD3Device().GetCommandList(COMMAND_LIST_UPDATE)->CopyResource(m_buffer_.Get(), upload_buffer.Get());

        GetGC().Track(upload_buffer);
      }

      const auto& cb_trans = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

      GetD3Device().GetCommandList(COMMAND_LIST_UPDATE)->ResourceBarrier(1, &cb_trans);

      const D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc
      {
        .BufferLocation = m_buffer_->GetGPUVirtualAddress(),
        .SizeInBytes    = m_alignment_size_
      };

      GetD3Device().ExecuteCommandList(COMMAND_LIST_UPDATE);

      constexpr D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc
      {
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = 1,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask       = 0
      };

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateDescriptorHeap(&cbv_heap_desc, IID_PPV_ARGS(m_cpu_cbv_heap_.GetAddressOf()))
        );

      GetD3Device().GetDevice()->CreateConstantBufferView
        (
         &cbv_desc,
         m_cpu_cbv_heap_->GetCPUDescriptorHandleForHeapStart()
        );

    }

    void SetData(const T* src_data)
    {
      GetD3Device().WaitAndReset(COMMAND_LIST_UPDATE);

      const auto& copy_trans = CD3DX12_RESOURCE_BARRIER::Transition(
          m_buffer_.Get(), 
          D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 
          D3D12_RESOURCE_STATE_COPY_DEST);

      GetD3Device().GetCommandList(COMMAND_LIST_UPDATE)->ResourceBarrier(1, &copy_trans);

      // Use upload buffer for synchronization.
      ComPtr<ID3D12Resource> upload_buffer;

      const auto&            upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
      const auto&            buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(m_alignment_size_);

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

      const auto         gen_type_name = std::string(typeid(T).name());
      const auto         type_name     = std::wstring(gen_type_name.begin(), gen_type_name.end());
      const std::wstring upload_buffer_name = type_name + L" Constant Buffer Upload Buffer";

      DX::ThrowIfFailed(upload_buffer->SetName(upload_buffer_name.c_str()));

      char* data = nullptr;

      DX::ThrowIfFailed(upload_buffer->Map(0, nullptr, reinterpret_cast<void**
      >(&data)));

      std::memcpy(data, src_data, sizeof(T));

      upload_buffer->Unmap(0, nullptr);

      GetD3Device().GetCommandList(COMMAND_LIST_UPDATE)->CopyResource(m_buffer_.Get(), upload_buffer.Get());

      const auto& cb_trans = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

      GetD3Device().GetCommandList(COMMAND_LIST_UPDATE)->ResourceBarrier(1, &cb_trans);

      GetD3Device().ExecuteCommandList(COMMAND_LIST_UPDATE);

      GetGC().Track(upload_buffer);
    }

    void Bind()
    {
      GetD3Device().BindConstantBufferView(which_cb<T>::value, m_cpu_cbv_heap_->GetCPUDescriptorHandleForHeapStart());
    }

  private:
    ComPtr<ID3D12DescriptorHeap> m_cpu_cbv_heap_;
    ComPtr<ID3D12Resource> m_buffer_;
    UINT m_alignment_size_;

  };

  template <typename T>
  ConstantBuffer<T>::ConstantBuffer()
  {
    static_assert(std::is_standard_layout_v<T>, "Constant buffer type must be a POD type");

    m_alignment_size_ = (sizeof(T) + 255) & ~255;
  }
}

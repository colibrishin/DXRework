#pragma once

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

  template <typename T>
  class ConstantBuffer
  {
  public:
    ConstantBuffer();

    void Create(const T* src_data)
    {
      DirectCommandGuard dcg;

      const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
      const auto& cb_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T));

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

      const auto& cb_trans = CD3DX12_RESOURCE_BARRIER::Transition(m_buffer_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

      GetD3Device().GetCommandList()->ResourceBarrier(1, &cb_trans);
    }

    void SetData(const T* src_data)
    {
      DirectCommandGuard dcg;

      const auto& copy_trans = CD3DX12_RESOURCE_BARRIER::Transition(m_buffer_.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);

      GetD3Device().GetCommandList()->ResourceBarrier(1, &copy_trans);

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

      constexpr D3D12_DESCRIPTOR_HEAP_DESC heap_desc
      {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = 1,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask = 0
      };

      DX::ThrowIfFailed
        (GetD3Device().GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(m_cbv_heap_.GetAddressOf())));

      m_cbv_desc_.BufferLocation = m_buffer_->GetGPUVirtualAddress();
      m_cbv_desc_.SizeInBytes = sizeof(T);

      GetD3Device().GetDevice()->CreateConstantBufferView(
          &m_cbv_desc_, 
          m_cbv_heap_->GetCPUDescriptorHandleForHeapStart());
    }

    void Bind() const
    {
      DirectCommandGuard dcg;

      GetD3Device().GetCommandList()->SetGraphicsRootConstantBufferView(which_cb<T>::value, m_buffer_->GetGPUVirtualAddress());
    }

    void Unbind() const
    {
      DirectCommandGuard dcg;

      GetD3Device().GetCommandList()->SetGraphicsRootConstantBufferView(which_cb<T>::value, 0);
    }

  private:

    ComPtr<ID3D12DescriptorHeap> m_cbv_heap_;
    ComPtr<ID3D12Resource> m_buffer_;
    D3D12_CONSTANT_BUFFER_VIEW_DESC m_cbv_desc_;

  };

  template <typename T>
  ConstantBuffer<T>::ConstantBuffer()
    : m_cbv_desc_()
  {
    static_assert(sizeof(T) % 256 == 0, "Constant buffer size must be 256-byte aligned");
    static_assert(std::is_pod_v<T>, "Constant buffer type must be a POD type");
  }
}

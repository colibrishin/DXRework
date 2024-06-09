#pragma once
#include <d3dx12.h>

#include "egD3Device.hpp"
#include "egDescriptors.h"
#include "egGarbageCollector.h"
#include "egGlobal.h"

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
      const auto& cmd = GetD3Device().AcquireCommandPair(L"ConstantBuffer Initialization").lock();

      cmd->SoftReset();

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

      const auto& upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
      const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(m_alignment_size_);

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateCommittedResource
         (
          &upload_heap,
          D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
          &buffer_desc,
          D3D12_RESOURCE_STATE_GENERIC_READ,
          nullptr,
          IID_PPV_ARGS(m_upload_buffer_.GetAddressOf())
         )
        );

      const std::wstring upload_buffer_name = type_name + L" Constant Buffer Upload Buffer";

      DX::ThrowIfFailed(m_upload_buffer_->SetName(upload_buffer_name.c_str()));

      if (src_data != nullptr)
      {
        char* data = nullptr;

        DX::ThrowIfFailed(m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
        std::memcpy(data, src_data, sizeof(T));
        m_upload_buffer_->Unmap(0, nullptr);

        cmd->GetList()->CopyResource(m_buffer_.Get(), m_upload_buffer_.Get());
      }

      const auto& cb_trans = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_buffer_.Get(),
         D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

      cmd->GetList()->ResourceBarrier(1, &cb_trans);

      const D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc
      {
        .BufferLocation = m_buffer_->GetGPUVirtualAddress(),
        .SizeInBytes    = m_alignment_size_
      };

      cmd->FlagReady();

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

      m_b_dirty_ = false;
    }

    void SetData(const T* src_data)
    {
      if (src_data != nullptr)
      {
        m_data_ = *src_data;
      }

      m_b_dirty_ = true;
    }

    T GetData() const
    {
      return m_data_;
    }

    void Bind(const Weak<CommandPair>& w_cmd, const DescriptorPtr& heap)
    {
      if (m_b_dirty_)
      {
        const auto& cmd = w_cmd.lock();

        const auto& copy_trans = CD3DX12_RESOURCE_BARRIER::Transition
          (
           m_buffer_.Get(),
           D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
           D3D12_RESOURCE_STATE_COPY_DEST
          );

        char* data = nullptr;

        DX::ThrowIfFailed(m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));

        std::memcpy(data, &m_data_, sizeof(T));

        m_upload_buffer_->Unmap(0, nullptr);

        const auto& cb_trans = CD3DX12_RESOURCE_BARRIER::Transition
          (
           m_buffer_.Get(),
           D3D12_RESOURCE_STATE_COPY_DEST,
           D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
          );

        cmd->GetList()->ResourceBarrier(1, &copy_trans);
        cmd->GetList()->CopyResource(m_buffer_.Get(), m_upload_buffer_.Get());
        cmd->GetList()->ResourceBarrier(1, &cb_trans);

        m_b_dirty_ = false;
      }

      heap->SetConstantBuffer(m_cpu_cbv_heap_->GetCPUDescriptorHandleForHeapStart(), which_cb<T>::value);
    }

  private:
    T                            m_data_;
    bool                         m_b_dirty_;
    ComPtr<ID3D12DescriptorHeap> m_cpu_cbv_heap_;
    ComPtr<ID3D12Resource>       m_upload_buffer_;
    ComPtr<ID3D12Resource>       m_buffer_;
    UINT                         m_alignment_size_;

  };

  template <typename T>
  ConstantBuffer<T>::ConstantBuffer()
    : m_b_dirty_(false)
  {
    static_assert(std::is_standard_layout_v<T>, "Constant buffer type must be a POD type");

    m_alignment_size_ = (sizeof(T) + 255) & ~255;
  }
}

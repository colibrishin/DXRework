#include "pch.h"
#include "egDescriptors.h"

namespace Engine
{
  DescriptorPtrImpl::DescriptorPtrImpl()
    : m_handler_(nullptr),
      m_offset_(0),
      m_cpu_handle_(),
      m_gpu_handle_(),
      m_cpu_sampler_handle_(),
      m_gpu_sampler_handle_(),
      m_buffer_descriptor_size_(0),
      m_sampler_descriptor_size_(0) {}

  DescriptorPtrImpl::DescriptorPtrImpl(DescriptorPtrImpl&& other) noexcept
  {
    m_handler_                 = std::move(other.m_handler_);
    m_offset_                  = std::move(other.m_offset_);
    m_cpu_handle_              = std::move(other.m_cpu_handle_);
    m_gpu_handle_              = std::move(other.m_gpu_handle_);
    m_cpu_sampler_handle_      = std::move(other.m_cpu_sampler_handle_);
    m_gpu_sampler_handle_      = std::move(other.m_gpu_sampler_handle_);
    m_buffer_descriptor_size_  = std::move(other.m_buffer_descriptor_size_);
    m_sampler_descriptor_size_ = std::move(other.m_sampler_descriptor_size_);
  }

  DescriptorPtrImpl& DescriptorPtrImpl::operator=(DescriptorPtrImpl&& other) noexcept
  {
    if (this != &other)
    {
      m_handler_                 = std::move(other.m_handler_);
      m_offset_                  = std::move(other.m_offset_);
      m_cpu_handle_              = std::move(other.m_cpu_handle_);
      m_gpu_handle_              = std::move(other.m_gpu_handle_);
      m_cpu_sampler_handle_      = std::move(other.m_cpu_sampler_handle_);
      m_gpu_sampler_handle_      = std::move(other.m_gpu_sampler_handle_);
      m_buffer_descriptor_size_  = std::move(other.m_buffer_descriptor_size_);
      m_sampler_descriptor_size_ = std::move(other.m_sampler_descriptor_size_);
    }

    return *this;
  }

  DescriptorPtrImpl::~DescriptorPtrImpl()
  {
    if (m_handler_ != nullptr)
    {
      m_handler_->Release(*this);
    }
  }

  ID3D12DescriptorHeap* DescriptorPtrImpl::GetMainDescriptorHeap() const
  {
    return m_handler_->GetMainDescriptorHeap();
  }

  void                DescriptorPtrImpl::SetSampler(const D3D12_CPU_DESCRIPTOR_HANDLE& sampler, const UINT slot) const {
    const auto& handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
      (
       m_cpu_sampler_handle_,
       slot,
       m_sampler_descriptor_size_
      );

    GetD3Device().GetDevice()->CopyDescriptorsSimple
      (
       1,
       handle,
       sampler,
       D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
      );
  }

  void DescriptorPtrImpl::SetConstantBuffer(const D3D12_CPU_DESCRIPTOR_HANDLE& cbv, const UINT slot) const {
    const CD3DX12_CPU_DESCRIPTOR_HANDLE cbv_handle
      (
       m_cpu_handle_,
       g_cb_offset + slot,
       m_buffer_descriptor_size_
      );

    GetD3Device().GetDevice()->CopyDescriptorsSimple
      (
       1,
       cbv_handle,
       cbv,
       D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
      );
  }

  void DescriptorPtrImpl::SetShaderResource(const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle, const UINT slot) const {
    const CD3DX12_CPU_DESCRIPTOR_HANDLE heap_handle
      (
       m_cpu_handle_,
       g_srv_offset + slot,
       m_buffer_descriptor_size_
      );

    GetD3Device().GetDevice()->CopyDescriptorsSimple
      (
       1,
       heap_handle,
       srv_handle,
       D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
      );
  }

  void DescriptorPtrImpl::SetShaderResources(
    UINT slot, UINT count, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& data
  ) const {
    CD3DX12_CPU_DESCRIPTOR_HANDLE heap_handle
      (
       m_cpu_handle_,
       g_srv_offset + slot,
       m_buffer_descriptor_size_
      );

    for (INT i = 0; i < count; ++i)
    {
      GetD3Device().GetDevice()->CopyDescriptorsSimple
        (
         1,
         heap_handle,
         data[i],
         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );

      heap_handle.Offset(1, m_buffer_descriptor_size_);
    }
  }

  void DescriptorPtrImpl::SetUnorderedAccess(const D3D12_CPU_DESCRIPTOR_HANDLE& uav, const UINT slot) const {
    const CD3DX12_CPU_DESCRIPTOR_HANDLE uav_handle
      (
       m_cpu_handle_,
       g_uav_offset + slot,
       m_buffer_descriptor_size_
      );

    GetD3Device().GetDevice()->CopyDescriptorsSimple
      (
       1,
       uav_handle,
       uav,
       D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
      );
  }

  void DescriptorPtrImpl::BindGraphic(const CommandPair& cmd) const
  {
    const auto& command_list = cmd.GetList();

    const auto& type = cmd.GetType();

    if (type == COMMAND_TYPE_DIRECT)
    {
      command_list->SetGraphicsRootSignature(GetRenderPipeline().GetRootSignature());

      ID3D12DescriptorHeap* heaps[]
      {
        m_handler_->GetMainDescriptorHeap(),
        m_handler_->GetMainSamplerDescriptorHeap()
      };

      command_list->SetDescriptorHeaps(2, heaps);

      command_list->SetGraphicsRootDescriptorTable
        (
         DESCRIPTOR_SLOT_SAMPLER,
         m_gpu_sampler_handle_
        );

      command_list->SetGraphicsRootDescriptorTable
        (
         DESCRIPTOR_SLOT_SRV,
         m_gpu_handle_
        );

      CD3DX12_GPU_DESCRIPTOR_HANDLE cb_handle
        (
         m_gpu_handle_,
         g_cb_offset,
         m_buffer_descriptor_size_
        );

      command_list->SetGraphicsRootDescriptorTable
        (
         DESCRIPTOR_SLOT_CB,
         cb_handle
        );

      cb_handle.Offset(g_uav_offset - g_cb_offset, m_buffer_descriptor_size_);

      command_list->SetGraphicsRootDescriptorTable
        (
         DESCRIPTOR_SLOT_UAV,
         cb_handle
        );
    }
    else if (type == COMMAND_TYPE_COMPUTE)
    {
      command_list->SetComputeRootSignature(GetRenderPipeline().GetRootSignature());

      ID3D12DescriptorHeap* heaps[]
      {
        m_handler_->GetMainDescriptorHeap(),
        m_handler_->GetMainSamplerDescriptorHeap()
      };

      command_list->SetDescriptorHeaps(2, heaps);

      command_list->SetComputeRootDescriptorTable
        (
         DESCRIPTOR_SLOT_SAMPLER,
         m_gpu_sampler_handle_
        );

      command_list->SetComputeRootDescriptorTable
        (
         DESCRIPTOR_SLOT_SRV,
         m_gpu_handle_
        );

      CD3DX12_GPU_DESCRIPTOR_HANDLE cb_handle
        (
         m_gpu_handle_,
         g_cb_offset,
         m_buffer_descriptor_size_
        );

      command_list->SetComputeRootDescriptorTable
        (
         DESCRIPTOR_SLOT_CB,
         cb_handle
        );

      cb_handle.Offset(g_uav_offset - g_cb_offset, m_buffer_descriptor_size_);

      command_list->SetComputeRootDescriptorTable
        (
         DESCRIPTOR_SLOT_UAV,
         cb_handle
        );
    }
  }

  DescriptorHandler::DescriptorHandler()
  {
    constexpr D3D12_DESCRIPTOR_HEAP_DESC buffer_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = g_max_engine_texture_slots * g_max_concurrent_command_lists,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0
    };

    constexpr D3D12_DESCRIPTOR_HEAP_DESC buffer_heap_desc_sampler
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
      .NumDescriptors = g_max_sampler_slots * g_max_concurrent_command_lists,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &buffer_heap_desc,
        IID_PPV_ARGS(m_main_descriptor_heap_.GetAddressOf())
       )
      );

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &buffer_heap_desc_sampler,
        IID_PPV_ARGS(m_main_sampler_descriptor_heap_.GetAddressOf())
       )
      );

    m_buffer_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_sampler_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  }

  DescriptorPtr DescriptorHandler::Acquire()
  {
    for (UINT i = 0; i < g_max_concurrent_command_lists; ++i)
    {
      if (!m_used_slots_[i].exchange(true))
      {
        const auto& buffer_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
          (
           m_main_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
           i * g_max_engine_texture_slots,
           m_buffer_size_
          );

        const auto& sampler_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
          (
           m_main_sampler_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
           i * g_max_sampler_slots,
           m_sampler_size_
          );

        return DescriptorPtr(new DescriptorPtrImpl
          (
           this,
           i,
           buffer_handle,
           CD3DX12_GPU_DESCRIPTOR_HANDLE
           (
            m_main_descriptor_heap_->GetGPUDescriptorHandleForHeapStart(),
            i * g_max_engine_texture_slots,
            m_buffer_size_
           ),
           sampler_handle,
           CD3DX12_GPU_DESCRIPTOR_HANDLE
           (
            m_main_sampler_descriptor_heap_->GetGPUDescriptorHandleForHeapStart(),
            i * g_max_sampler_slots,
            m_sampler_size_
           ),
           m_buffer_size_,
           m_sampler_size_
          ));
      }
    }

    return {};
  }

  void DescriptorHandler::Release(const DescriptorPtrImpl& handles)
  {
    m_used_slots_[handles.m_offset_].store(false);
  }
}

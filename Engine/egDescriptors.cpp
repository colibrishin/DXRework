#include "pch.h"
#include "egDescriptors.h"

namespace Engine
{
  DescriptorPtrImpl::DescriptorPtrImpl()
    : m_handler_(nullptr),
      m_segment_offset_(-1),
      m_element_offset_(-1),
      m_heap_queue_offset_(-1),
      m_cpu_handle_(),
      m_gpu_handle_(),
      m_cpu_sampler_handle_(),
      m_gpu_sampler_handle_(),
      m_buffer_descriptor_size_(0),
      m_sampler_descriptor_size_(0) {}

  DescriptorPtrImpl::DescriptorPtrImpl(DescriptorPtrImpl&& other) noexcept
  {
    m_handler_                 = std::move(other.m_handler_);
    m_segment_offset_          = std::move(other.m_segment_offset_);
    m_element_offset_          = std::move(other.m_element_offset_);
    m_heap_queue_offset_       = std::move(other.m_heap_queue_offset_);
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
      m_segment_offset_          = std::move(other.m_segment_offset_);
      m_element_offset_          = std::move(other.m_element_offset_);
      m_heap_queue_offset_       = std::move(other.m_heap_queue_offset_);
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
    Release();
  }

  bool DescriptorPtrImpl::IsValid() const
  {
    return m_heap_queue_offset_ != -1 && m_segment_offset_ != -1 && m_element_offset_ != -1;
  }

  void DescriptorPtrImpl::Release() const
  {
    if (IsValid() && m_handler_ != nullptr)
    {
      m_handler_->Release(*this);
    }
  }

  ID3D12DescriptorHeap* DescriptorPtrImpl::GetMainDescriptorHeap() const
  {
    if (!IsValid())
    {
      return nullptr;
    }

    return m_handler_->GetMainDescriptorHeap(m_heap_queue_offset_);
  }

  void DescriptorPtrImpl::SetSampler(const D3D12_CPU_DESCRIPTOR_HANDLE& sampler, const UINT slot) const
  {
    if (!IsValid())
    {
      return;
    }

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

  void DescriptorPtrImpl::SetConstantBuffer(const D3D12_CPU_DESCRIPTOR_HANDLE& cbv, const UINT slot) const
  {
    if (!IsValid())
    {
      return;
    }

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

  void DescriptorPtrImpl::SetShaderResource(const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle, const UINT slot) const
  {
    if (!IsValid())
    {
      return;
    }

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
  ) const
  {
    if (!IsValid())
    {
      return;
    }

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

  void DescriptorPtrImpl::SetUnorderedAccess(const D3D12_CPU_DESCRIPTOR_HANDLE& uav, const UINT slot) const
  {
    if (!IsValid())
    {
      return;
    }

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

  void DescriptorPtrImpl::BindGraphic(const Weak<CommandPair>& w_cmd) const
  {
    if (!IsValid())
    {
      return;
    }

    const auto& cmd = w_cmd.lock();
    const auto& command_list = cmd->GetList();
    const auto& type = cmd->GetType();

    if (type == COMMAND_TYPE_DIRECT)
    {
      command_list->SetGraphicsRootSignature(GetRenderPipeline().GetRootSignature());

      ID3D12DescriptorHeap* heaps[]
      {
        m_handler_->GetMainDescriptorHeap(m_heap_queue_offset_),
        m_handler_->GetMainSamplerDescriptorHeap(m_heap_queue_offset_)
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
        m_handler_->GetMainDescriptorHeap(m_heap_queue_offset_),
        m_handler_->GetMainSamplerDescriptorHeap(m_heap_queue_offset_)
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

  void DescriptorHandler::AppendNewHeaps()
  {
    const D3D12_DESCRIPTOR_HEAP_DESC buffer_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = g_total_engine_slots * m_size_,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0
    };

    const D3D12_DESCRIPTOR_HEAP_DESC buffer_heap_desc_sampler
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
      .NumDescriptors = g_max_sampler_slots * m_size_,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0
    };

    ComPtr<ID3D12DescriptorHeap> buffer_heap;
    ComPtr<ID3D12DescriptorHeap> sampler_heap;

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &buffer_heap_desc,
        IID_PPV_ARGS(buffer_heap.GetAddressOf())
       )
      );

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &buffer_heap_desc_sampler,
        IID_PPV_ARGS(sampler_heap.GetAddressOf())
       )
      );

    m_main_descriptor_heap_.emplace_back(buffer_heap);
    m_main_sampler_descriptor_heap_.emplace_back(sampler_heap);

    m_used_slots_.push_back({});
    m_descriptors_.push_back({});

    m_descriptors_.back().resize(m_size_);
  }

  DescriptorHandler::DescriptorHandler()
  {
    // This value is 256 for matching with SIMD type size. (__m256i)
    m_size_ = 256;
    AppendNewHeaps();
    m_buffer_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_sampler_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  }

  DescriptorPtr DescriptorHandler::Acquire()
  {
    UINT64 queue_offset = 0;
    UINT64 segment_offset = 0;
    UINT64 element_offset = 0;
    bool found = false;

    if (check_avx())
    {
      static const __m256i all_mask = _mm256_set1_epi32(0xFFFFFFFF);

      for (int i = 0; i < m_used_slots_.size(); ++i)
      {
        // Bitwise AND + NOT
        // If NOT + zero operand then carry flag is set
        // AND => 1 => NOT => CF = false (which means, all of bits are set)
        // AND => 0 => NOT => CF = true (which means, all of bits are not set)
        if (!_mm256_testc_si256(m_used_slots_[i], all_mask))
        {
          queue_offset = i;

          for (int j = 0; j < s_segment_size; ++j)
          {
            const auto& segment_bits = m_used_slots_[i].m256i_u32[j];

            // Not available if all bits are set.
            if (segment_bits == 0xFFFFFFFF) { continue; }

            segment_offset = j;
            // Trailing zero (first available slot, negates segment bits for using zero)
            element_offset = _tzcnt_u32(~segment_bits);

            found = true;
            break;
          }
        }

        if (found)
        {
          break;
        }

        ++queue_offset;
      }
    }
    else
    {
      // SSE2 fallback
      static const __m128i all_mask = _mm_set1_epi32(0xFFFFFFFF);

      for (int i = 0; i < m_used_slots_.size(); ++i)
      {
        for (int j = 0; j < sizeof(__m256i) / sizeof(__m128); ++j)
        {
          const auto& mask = _mm_cmpeq_epi32(reinterpret_cast<__m128i&>(m_used_slots_[i]), all_mask);

          for (int k = 0; k < sizeof(__m128i) / sizeof(UINT32); ++k)
          {
            if (mask.m128i_u32[k] == 0xFFFFFFFF)
            {
              continue;
            }

            segment_offset = (j + 1) * k;
            element_offset = _tzcnt_u32(~m_used_slots_[i].m256i_u32[segment_offset]);

            found = true;
            break;
          }

          if (found)
          {
            break;
          }
        }

        if (found)
        {
          break;
        }

        ++queue_offset;
      }
    }
    
    if (queue_offset >= m_used_slots_.size())
    {
      OutputDebugStringA("WARNING: DescriptorHandler No available slots. Appending new heaps.\n");
      AppendNewHeaps();
    }

    const auto& idx                      = segment_offset * s_element_size + element_offset;
    const auto& buffer_heap_side_offset  = g_total_engine_slots * idx;
    const auto& sampler_heap_side_offset = g_max_sampler_slots * idx;

    const auto& buffer_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
      (
       m_main_descriptor_heap_[queue_offset]->GetCPUDescriptorHandleForHeapStart(),
       buffer_heap_side_offset,
       m_buffer_size_
      );

    const auto& sampler_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
      (
       m_main_sampler_descriptor_heap_[queue_offset]->GetCPUDescriptorHandleForHeapStart(),
       sampler_heap_side_offset,
       m_sampler_size_
      );

    const auto& gpu_buffer_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE
      (
       m_main_descriptor_heap_[queue_offset]->GetGPUDescriptorHandleForHeapStart(),
       buffer_heap_side_offset,
       m_buffer_size_
      );

    const auto& gpu_sampler_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE
      (
       m_main_sampler_descriptor_heap_[queue_offset]->GetGPUDescriptorHandleForHeapStart(),
       sampler_heap_side_offset,
       m_sampler_size_
      );

    m_descriptors_[queue_offset][idx] = StrongDescriptorPtr
      (
       new DescriptorPtrImpl
       (
        this,
        queue_offset,
        segment_offset,
        element_offset,
        buffer_handle,
        gpu_buffer_handle,
        sampler_handle,
        gpu_sampler_handle,
        m_buffer_size_,
        m_sampler_size_
       )
      );

    m_used_slots_[queue_offset].m256i_i32[segment_offset] = m_used_slots_[queue_offset].m256i_i32[segment_offset] | (1 << element_offset);

    return m_descriptors_[queue_offset][idx];
  }

  void DescriptorHandler::Release(const DescriptorPtrImpl& handles)
  {
    m_used_slots_[handles.m_heap_queue_offset_].m256i_u32[handles.m_segment_offset_] = m_used_slots_[handles.m_heap_queue_offset_].m256i_u32[handles.m_segment_offset_] & ~(1 << handles.m_element_offset_);

    // Dispose the handle.
    const auto prev_queue_offset = handles.m_heap_queue_offset_;
    const auto prev_segment_offset = handles.m_segment_offset_;
    const auto prev_offset = handles.m_element_offset_;
    const auto& idx = prev_segment_offset * s_element_size + prev_offset;

    m_descriptors_[prev_queue_offset][idx]->m_cpu_handle_ = {};
    m_descriptors_[prev_queue_offset][idx]->m_gpu_handle_ = {};
    m_descriptors_[prev_queue_offset][idx]->m_cpu_sampler_handle_ = {};
    m_descriptors_[prev_queue_offset][idx]->m_gpu_sampler_handle_ = {};
    m_descriptors_[prev_queue_offset][idx]->m_segment_offset_ = -1;
    m_descriptors_[prev_queue_offset][idx]->m_element_offset_ = -1;
    m_descriptors_[prev_queue_offset][idx]->m_heap_queue_offset_ = -1;
    m_descriptors_[prev_queue_offset][idx].reset();
  }

  ID3D12DescriptorHeap* DescriptorHandler::GetMainDescriptorHeap(const UINT64 offset) const
  {
    return m_main_descriptor_heap_[offset].Get();
  }

  ID3D12DescriptorHeap* DescriptorHandler::GetMainSamplerDescriptorHeap(const UINT64 offset) const
  {
    return m_main_sampler_descriptor_heap_[offset].Get();
  }
}

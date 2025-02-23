#pragma once
#include <directx/d3d12.h>
#include <wrl/client.h>


#include "GraphicInterface.h"

#include "DescriptorPtrImpl.h"
#include "SIMDExtension.hpp"

namespace Engine
{
    using DescriptorPtr = Unique<DescriptorPtrImpl>;
    
    struct ENGINE_D3D12GRAPHICINTERFACE_API DescriptorHandlerBase
    {
        virtual ~DescriptorHandlerBase() = default;

        DescriptorHandlerBase()
        {
            m_size_ = 256;
        }

        virtual void Initialize(ID3D12Device2* dev, ID3D12RootSignature* root_signature)
        {
            m_dev_           = dev;
            m_root_signature = root_signature;

            AppendNewHeaps();
            m_buffer_size_  = m_dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            m_sampler_size_ = m_dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        }

        virtual DescriptorPtr Acquire() = 0;

        [[nodiscard]] virtual bool IsValid(const DescriptorPtrImpl* ptr)
        {
            return ptr->m_heap_queue_offset_ != -1 && ptr->m_segment_offset_ != -1 && ptr->m_element_offset_ != -1 &&
                (m_used_slots_[ptr->m_heap_queue_offset_].m256i_i32[ptr->m_segment_offset_] & (1 << ptr->m_element_offset_)) != 0;
        }

        virtual void Release(const DescriptorPtrImpl& handles)
        {
            m_used_slots_[handles.m_heap_queue_offset_].m256i_u32[handles.m_segment_offset_] &= ~(
                1 << handles.m_element_offset_);
        }

        [[nodiscard]] virtual UINT64 GetBufferHeapGPUAddress(const D3D12_GPU_DESCRIPTOR_HANDLE& m_gpu_handle, const size_t size) = 0;
        [[nodiscard]] virtual UINT64 GetSamplerHeapGPUAddress(const D3D12_GPU_DESCRIPTOR_HANDLE& m_gpu_handle, const size_t size) = 0;

        virtual void SetSampler(
            const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& sampler, UINT slot
        ) const = 0;
        virtual void SetConstantBuffer(
            const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& cbv, UINT slot
        ) const = 0;
        virtual void SetShaderResource(
            const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle, UINT slot
        ) const = 0;
        virtual void SetShaderResources(
            const D3D12_CPU_DESCRIPTOR_HANDLE&              begin, UINT slot, UINT count,
            const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& data
        ) const = 0;
        virtual void SetUnorderedAccess(
            const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& uav, UINT slot
        ) const = 0;
        virtual void BindGraphic(
            const GraphicInterfaceContextPrimitive* context, ID3D12DescriptorHeap*                   buffer_heap,
            ID3D12DescriptorHeap*                   sampler_heap, const D3D12_GPU_DESCRIPTOR_HANDLE& buffer_handle,
            const D3D12_GPU_DESCRIPTOR_HANDLE&      sampler_handle
        ) const = 0;
        virtual void BindCompute(
            const GraphicInterfaceContextPrimitive* context, ID3D12DescriptorHeap*                   buffer_heap,
            ID3D12DescriptorHeap*                   sampler_heap, const D3D12_GPU_DESCRIPTOR_HANDLE& buffer_handle,
            const D3D12_GPU_DESCRIPTOR_HANDLE&      sampler_handle
        ) const = 0;

		[[nodiscard]] ID3D12DescriptorHeap* GetMainDescriptorHeap(UINT64 offset) const
		{
		    return m_main_descriptor_heap_[offset].Get();
		}
		[[nodiscard]] ID3D12DescriptorHeap* GetMainSamplerDescriptorHeap(UINT64 offset) const
		{
		    return m_main_sampler_descriptor_heap_[offset].Get();
		}
        [[nodiscard]] ID3D12RootSignature* GetRootSignature() const
        {
            return m_root_signature.Get();
        }

    private:
        virtual void AppendNewHeaps() = 0;
        
    protected:
        void Allocate(UINT64& queue_offset, UINT64& segment_offset, UINT64& element_offset)
        {
            bool   found          = false;

            if (SIMDExtension::check_avx())
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

                    if (found) { break; }

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
                            if (mask.m128i_u32[k] == 0xFFFFFFFF) { continue; }

                            segment_offset = (j + 1) * k;
                            element_offset = _tzcnt_u32(~m_used_slots_[i].m256i_u32[segment_offset]);

                            found = true;
                            break;
                        }

                        if (found) { break; }
                    }

                    if (found) { break; }

                    ++queue_offset;
                }
            }

            if (queue_offset >= m_used_slots_.size())
            {
                OutputDebugStringA("WARNING: DescriptorHandler No available slots. Appending new heaps.\n");
                AppendNewHeaps();
            }

            m_used_slots_[queue_offset].m256i_i32[segment_offset] =
                m_used_slots_[queue_offset].m256i_i32[segment_offset] | (1 << element_offset);
        }

        template <typename PolymorphicPointerImpl, typename... Args> requires std::is_base_of_v<PolymorphicPointerImpl, DescriptorPtrImpl>
        DescriptorPtr AllocatePtr(Args&&... args)
        {
            return Unique<DescriptorPtrImpl>(new PolymorphicPointerImpl(std::forward<Args>(args)...));
        }
        
        static constexpr size_t s_element_size = std::numeric_limits<unsigned int>::digits;
        static constexpr size_t s_segment_size = sizeof(__m256i) / sizeof(unsigned int);
        std::deque<__m256i>                                m_used_slots_{};

        ComPtr<ID3D12Device2>                              m_dev_{};
        ComPtr<ID3D12RootSignature>                        m_root_signature{};
        UINT                                               m_size_{};

        aligned_vector<ComPtr<ID3D12DescriptorHeap>> m_main_descriptor_heap_{};
        aligned_vector<ComPtr<ID3D12DescriptorHeap>> m_main_sampler_descriptor_heap_{};

        UINT m_buffer_size_{};
        UINT m_sampler_size_{};
    };
    
    template <typename HeapAllocator, typename HeapGetter, typename HeapBinder, typename PolymorphicPointerImpl = DescriptorPtrImpl>
	struct DescriptorHandler final : public DescriptorHandlerBase
	{
	public:
        DescriptorPtr Acquire() override
        {
            UINT64 queue_offset   = 0;
            UINT64 segment_offset = 0;
            UINT64 element_offset = 0;
            Allocate(queue_offset, segment_offset, element_offset);

            D3D12_CPU_DESCRIPTOR_HANDLE out_buffer_handle,     out_sampler_handle;
            D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_buffer_handle, out_gpu_sampler_handle;
            m_heap_getter_
                (
                 s_element_size, m_buffer_size_, m_sampler_size_, queue_offset, segment_offset, element_offset,
                 out_buffer_handle, out_sampler_handle, out_gpu_buffer_handle, out_gpu_sampler_handle,
                 m_main_descriptor_heap_, m_main_sampler_descriptor_heap_
                );

            return AllocatePtr<PolymorphicPointerImpl>
                (
                 this, queue_offset, segment_offset, element_offset, out_buffer_handle, out_gpu_buffer_handle,
                 out_sampler_handle, out_gpu_sampler_handle
                );
        }

	private:
        HeapAllocator m_heap_allocator_{};
        HeapGetter m_heap_getter_{};
        HeapBinder m_heap_binder_{};
        
		friend struct DescriptorPtrImpl;
		void AppendNewHeaps() override
        {
		    ComPtr<ID3D12DescriptorHeap> buffer_heap;
		    ComPtr<ID3D12DescriptorHeap> sampler_heap;
		    
		    m_heap_allocator_(m_dev_.Get(), buffer_heap.GetAddressOf(), sampler_heap.GetAddressOf(), m_size_);

		    m_main_descriptor_heap_.emplace_back(buffer_heap);
		    m_main_sampler_descriptor_heap_.emplace_back(sampler_heap);
		    m_used_slots_.push_back({});
		}

    public:
        void SetSampler(
            const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& sampler, UINT slot
        ) const override { m_heap_binder_.SetSampler(m_dev_.Get(), begin, sampler, slot, m_sampler_size_); }
        
        void SetConstantBuffer(
            const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& cbv, UINT slot
        ) const override { m_heap_binder_.SetConstantBuffer(m_dev_.Get(), begin, cbv, slot, m_buffer_size_); }
        
        void SetShaderResource(
            const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle, UINT slot
        ) const override { m_heap_binder_.SetShaderResource(m_dev_.Get(), begin, srv_handle, slot, m_buffer_size_); }
        
        void SetShaderResources(
            const D3D12_CPU_DESCRIPTOR_HANDLE&              begin, UINT slot, UINT count,
            const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& data
        ) const override { m_heap_binder_.SetShaderResources(m_dev_.Get(), begin, slot, count, data, m_buffer_size_); }
        
        void SetUnorderedAccess(
            const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& uav, UINT slot
        ) const override { m_heap_binder_.SetUnorderedAccess(m_dev_.Get(), begin, uav, slot, m_buffer_size_); }

        void BindGraphic(
            const GraphicInterfaceContextPrimitive* context, ID3D12DescriptorHeap*                   buffer_heap,
            ID3D12DescriptorHeap*                   sampler_heap, const D3D12_GPU_DESCRIPTOR_HANDLE& buffer_handle,
            const D3D12_GPU_DESCRIPTOR_HANDLE&      sampler_handle
        ) const override
        {
            m_heap_binder_.BindGraphic
                (
                 m_root_signature.Get(), context, buffer_heap, sampler_heap, buffer_handle, sampler_handle, m_buffer_size_,
                 m_sampler_size_
                );
        }

        void BindCompute(
            const GraphicInterfaceContextPrimitive* context, ID3D12DescriptorHeap*                   buffer_heap,
            ID3D12DescriptorHeap*                   sampler_heap, const D3D12_GPU_DESCRIPTOR_HANDLE& buffer_handle,
            const D3D12_GPU_DESCRIPTOR_HANDLE&      sampler_handle
        ) const override
        {
            m_heap_binder_.BindCompute
                (
                 m_root_signature.Get(), context, buffer_heap, sampler_heap, buffer_handle, sampler_handle, m_buffer_size_,
                 m_sampler_size_
                );
        }

        [[nodiscard]] UINT64 GetBufferHeapGPUAddress(const D3D12_GPU_DESCRIPTOR_HANDLE& begin, const size_t offset) override
        {
            return m_heap_getter_.GetBufferHeapGPUAddress(begin, offset, m_buffer_size_);
        }

        [[nodiscard]] UINT64 GetSamplerHeapGPUAddress(const D3D12_GPU_DESCRIPTOR_HANDLE& begin, const size_t offset) override
        {
            return m_heap_getter_.GetSamplerHeapGPUAddress(begin, offset, m_sampler_size_);
        }
    };
}

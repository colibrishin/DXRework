#pragma once
#include <array>
#include <atomic>
#include <stdexcept>

#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include "DescriptorHandler.hpp"
#include "FrameRelease.h"
#include "raytracing_heap_binder.hpp"

namespace Engine
{
    // Descriptor tables of the ray tracing hit records. DispatchRays reads them through the one CBV/SRV/UAV heap and
    // the one sampler heap bound to the list, so every instance's table and the output UAV live in one heap pair per
    // frame in flight: BeginFrame sizes this frame's pair for the table count, Acquire hands out tables in order.
    // All tables share the single sampler table (the pass binds the same shader sampler for every instance).
    struct raytracing_frame_heap final : DescriptorHandlerBase
    {
        void Initialize(ID3D12Device2* dev, ID3D12RootSignature* root_signature) override
        {
            m_dev_            = dev;
            m_root_signature_ = root_signature;
            m_buffer_size_    = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            m_sampler_size_   = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

            m_main_descriptor_heap_.resize(g_max_frames_in_flight);
            m_main_sampler_descriptor_heap_.resize(g_max_frames_in_flight);

            constexpr D3D12_DESCRIPTOR_HEAP_DESC sampler_desc{ .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                                                               .NumDescriptors = g_max_sampler_slots,
                                                               .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                                                               .NodeMask       = 0 };
            for (auto& heap : m_main_sampler_descriptor_heap_)
            {
                DX::ThrowIfFailed(dev->CreateDescriptorHeap(&sampler_desc, IID_PPV_ARGS(heap.GetAddressOf())));
            }
        }

        // Tables one shader-visible heap can hold on every resource binding tier (plus the colour and depth output UAVs).
        static constexpr UINT s_output_slots = 2;
        static constexpr UINT s_max_tables =
                (D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1 - s_output_slots) / g_local_raytracing_total_engine_slots;

        // Makes this frame's heap hold min(tables, s_max_tables) tables; returns that count and the CPU handle of the
        // output UAV slots (colour, then depth).
        UINT BeginFrame(UINT tables, D3D12_CPU_DESCRIPTOR_HANDLE& output)
        {
            tables = (std::min)(tables, s_max_tables);
            std::lock_guard<std::mutex> l(m_slot_mutex_);
            const uint64_t frame     = RecordingFrame();
            const size_t   slot      = frame % g_max_frames_in_flight;
            const bool     rewritten = m_frames_[slot] == frame; // a list recorded earlier this frame may read it
            m_frames_[slot]          = frame;

            if (!m_main_descriptor_heap_[slot] || m_capacity_[slot] < tables || rewritten)
            {
                KeepUntilFrameDone(std::move(m_main_descriptor_heap_[slot]));
                const UINT capacity = (std::min)((std::max)(tables + tables / 2, 64u), s_max_tables);
                const D3D12_DESCRIPTOR_HEAP_DESC desc{ .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                       .NumDescriptors = capacity * g_local_raytracing_total_engine_slots + s_output_slots,
                                                       .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                                                       .NodeMask = 0 };
                DX::ThrowIfFailed(m_dev_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_main_descriptor_heap_[slot].GetAddressOf())));
                m_capacity_[slot] = capacity;
            }

            m_slot_ = slot;
            m_next_.store(0);
            output = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_main_descriptor_heap_[slot]->GetCPUDescriptorHandleForHeapStart(),
                                                   OutputIndex(slot), m_buffer_size_);
            return tables;
        }

        [[nodiscard]] ID3D12DescriptorHeap* GetBufferHeap() const { return GetMainDescriptorHeap(m_slot_); }
        [[nodiscard]] ID3D12DescriptorHeap* GetSamplerHeap() const { return GetMainSamplerDescriptorHeap(m_slot_); }
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetOutputGPUHandle() const
        {
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(GetBufferHeap()->GetGPUDescriptorHandleForHeapStart(), OutputIndex(m_slot_),
                                                 m_buffer_size_);
        }

        DescriptorPtr Acquire() override
        {
            const UINT idx = m_next_.fetch_add(1);
            std::lock_guard<std::mutex> l(m_slot_mutex_);
            if (idx >= m_capacity_[m_slot_])
            {
                throw std::runtime_error("raytracing_frame_heap: more tables than reserved by BeginFrame");
            }

            ID3D12DescriptorHeap* heap    = m_main_descriptor_heap_[m_slot_].Get();
            ID3D12DescriptorHeap* sampler = m_main_sampler_descriptor_heap_[m_slot_].Get();
            const UINT            offset  = idx * g_local_raytracing_total_engine_slots;
            return AllocatePtr<DescriptorPtrImpl>(
                this, m_slot_, 0, idx,
                CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->GetCPUDescriptorHandleForHeapStart(), offset, m_buffer_size_),
                CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(), offset, m_buffer_size_),
                sampler->GetCPUDescriptorHandleForHeapStart(), sampler->GetGPUDescriptorHandleForHeapStart());
        }

        // Tables live until the frame's heap is reused, not per pointer.
        [[nodiscard]] bool IsValid(const DescriptorPtrImpl*) override { return true; }
        void Release(const DescriptorPtrImpl&) override {}

        [[nodiscard]] UINT64 GetBufferHeapGPUAddress(const D3D12_GPU_DESCRIPTOR_HANDLE& begin, const size_t offset) override
        {
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(begin, static_cast<UINT>(offset), m_buffer_size_).ptr;
        }
        [[nodiscard]] UINT64 GetSamplerHeapGPUAddress(const D3D12_GPU_DESCRIPTOR_HANDLE& begin, const size_t offset) override
        {
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(begin, static_cast<UINT>(offset), m_sampler_size_).ptr;
        }

        void SetSampler(const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& sampler, UINT slot) const override
        {
            m_binder_.SetSampler(m_dev_.Get(), begin, sampler, slot, m_sampler_size_);
        }
        void SetConstantBuffer(const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& cbv, UINT slot) const override
        {
            m_binder_.SetConstantBuffer(m_dev_.Get(), begin, cbv, slot, m_buffer_size_);
        }
        void SetShaderResource(const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& srv, UINT slot) const override
        {
            m_binder_.SetShaderResource(m_dev_.Get(), begin, srv, slot, m_buffer_size_);
        }
        void SetShaderResources(const D3D12_CPU_DESCRIPTOR_HANDLE& begin, UINT slot, UINT count,
                                const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& data) const override
        {
            m_binder_.SetShaderResources(m_dev_.Get(), begin, slot, count, data, m_buffer_size_);
        }
        void SetUnorderedAccess(const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& uav, UINT slot) const override
        {
            m_binder_.SetUnorderedAccess(m_dev_.Get(), begin, uav, slot, m_buffer_size_);
        }
        void BindGraphic(const IGraphicContext*, ID3D12DescriptorHeap*, ID3D12DescriptorHeap*, const D3D12_GPU_DESCRIPTOR_HANDLE&,
                         const D3D12_GPU_DESCRIPTOR_HANDLE&) const override {}
        void BindCompute(const IGraphicContext*, ID3D12DescriptorHeap*, ID3D12DescriptorHeap*, const D3D12_GPU_DESCRIPTOR_HANDLE&,
                         const D3D12_GPU_DESCRIPTOR_HANDLE&) const override {}

    private:
        void AppendNewHeaps() override {}

        [[nodiscard]] UINT OutputIndex(const size_t slot) const { return m_capacity_[slot] * g_local_raytracing_total_engine_slots; }

        raytracing_heap_binder                       m_binder_{};
        std::atomic<UINT>                            m_next_{ 0 };
        size_t                                       m_slot_ = 0;
        std::array<UINT, g_max_frames_in_flight>     m_capacity_{};
        std::array<uint64_t, g_max_frames_in_flight> m_frames_{ UINT64_MAX, UINT64_MAX };
    };
}

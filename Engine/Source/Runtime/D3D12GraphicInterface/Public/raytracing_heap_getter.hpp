#pragma once
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include "Allocator.h"

#include "TypeLibrary.h"

namespace Engine
{
    struct raytracing_heap_getter
    {
        void operator()(
            const UINT64 element_size, const UINT buffer_size, const UINT sampler_size,
            const UINT64 queue_offset, const UINT64 segment_offset, const UINT64 element_offset,
            D3D12_CPU_DESCRIPTOR_HANDLE& buffer_handle, D3D12_CPU_DESCRIPTOR_HANDLE& sampler_handle,
            D3D12_GPU_DESCRIPTOR_HANDLE& gpu_buffer_handle, D3D12_GPU_DESCRIPTOR_HANDLE& gpu_sampler_handle,
            const aligned_vector<ComPtr<ID3D12DescriptorHeap>>& main_descriptor_heap,
            const aligned_vector<ComPtr<ID3D12DescriptorHeap>>& main_sampler_descriptor_heap) const
        {
            const auto& idx                      = segment_offset * element_size + element_offset;
            const auto& buffer_heap_side_offset  = static_cast<UINT>(g_local_raytracing_total_engine_slots * idx);
            const auto& sampler_heap_side_offset = static_cast<UINT>(g_max_sampler_slots * idx);

            buffer_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
                (
                 main_descriptor_heap[queue_offset]->GetCPUDescriptorHandleForHeapStart(), buffer_heap_side_offset,
                 buffer_size
                );

            sampler_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
                (
                 main_sampler_descriptor_heap[queue_offset]->GetCPUDescriptorHandleForHeapStart(),
                 sampler_heap_side_offset,
                 sampler_size
                );

            gpu_buffer_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE
                (
                 main_descriptor_heap[queue_offset]->GetGPUDescriptorHandleForHeapStart(), buffer_heap_side_offset,
                 buffer_size
                );

            gpu_sampler_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE
                (
                 main_sampler_descriptor_heap[queue_offset]->GetGPUDescriptorHandleForHeapStart(),
                 sampler_heap_side_offset,
                 sampler_size
                );
        }

        UINT64 GetBufferHeapGPUAddress(const D3D12_GPU_DESCRIPTOR_HANDLE& begin, const size_t offset, const size_t buffer_size)
        {
            const auto& handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
                begin,
                (UINT)offset,
                buffer_size);

            return handle.ptr;
        }

        UINT64 GetSamplerHeapGPUAddress(const D3D12_GPU_DESCRIPTOR_HANDLE& begin, const size_t offset, const size_t buffer_size)
        {
            const auto& handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
                begin,
                (UINT)offset,
                buffer_size);

            return handle.ptr;
        }
    };
}

#pragma once
#include <directx/d3d12.h>
#include "ThrowIfFailed.h"
#include "TypeLibrary.h"

namespace Engine
{
    struct raytracing_heap_allocator
    {
        void operator()(
            ID3D12Device* dev, ID3D12DescriptorHeap** buffer_heap, ID3D12DescriptorHeap** sampler_heap, const UINT size
        ) const
        {
            const D3D12_DESCRIPTOR_HEAP_DESC buffer_heap_desc
            {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                .NumDescriptors = (UINT)g_local_raytracing_total_engine_slots * size,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                .NodeMask = 0
            };

            const D3D12_DESCRIPTOR_HEAP_DESC buffer_heap_desc_sampler
            {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                .NumDescriptors = g_max_sampler_slots * size,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                .NodeMask = 0
            };

            DX::ThrowIfFailed
                (
                dev->CreateDescriptorHeap
                 (
                  &buffer_heap_desc,
                  IID_PPV_ARGS(buffer_heap)
                 )
                );

            DX::ThrowIfFailed
                    (
                    dev->CreateDescriptorHeap
                     (
                      &buffer_heap_desc_sampler,
                      IID_PPV_ARGS(sampler_heap)
                     )
                    );
        }
    };
}

#pragma once
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include "CommandPair.h"
#include "Texture.h"
#include "TypeLibrary.h"

namespace Engine
{
    struct raytracing_heap_binder
    {
        void SetSampler(
            ID3D12Device* dev, const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& sampler,
            UINT          slot, const UINT                        sampler_size
        ) const
        {
            const auto& handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(begin, slot, sampler_size);

            dev->CopyDescriptorsSimple(1, handle, sampler, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        }

        void SetConstantBuffer(
            ID3D12Device* dev, const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& cbv,
            UINT          slot, const UINT                        buffer_size
        ) const
        {
            return;
        }

        void SetShaderResource(
            ID3D12Device* dev, const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle,
            UINT          slot, const UINT                        buffer_size
        ) const
        {
            const CD3DX12_CPU_DESCRIPTOR_HANDLE heap_handle(begin, g_local_raytracing_srv_offset + slot, buffer_size);

            dev->CopyDescriptorsSimple(1, heap_handle, srv_handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        void SetShaderResources(
            ID3D12Device* dev, const D3D12_CPU_DESCRIPTOR_HANDLE& begin, UINT slot, UINT count,
            const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& data, const UINT buffer_size
        ) const
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE heap_handle(begin, g_local_raytracing_uav_offset + slot, buffer_size);

            for (UINT i = 0; i < count; ++i)
            {
                dev->CopyDescriptorsSimple(1, heap_handle, data[i], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                heap_handle.Offset(1, buffer_size);
            }
        }

        void SetUnorderedAccess(
            ID3D12Device* dev, const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& uav,
            UINT          slot, const UINT                        buffer_size
        ) const
        {
            const CD3DX12_CPU_DESCRIPTOR_HANDLE heap_handle(begin, g_local_raytracing_uav_offset + slot, buffer_size);

            dev->CopyDescriptorsSimple(1, heap_handle, uav, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        void BindGraphic(
            ID3D12RootSignature*               root_signature, const IGraphicContext* context,
            ID3D12DescriptorHeap*              buffer_heap, ID3D12DescriptorHeap*                      sampler_heap,
            const D3D12_GPU_DESCRIPTOR_HANDLE& buffer_handle, const D3D12_GPU_DESCRIPTOR_HANDLE&       sampler_handle,
            const UINT                         buffer_size, const UINT                                 sampler_size
        ) const
        {
            return;
        }

        void BindCompute(
            ID3D12RootSignature*               root_signature, const IGraphicContext* context,
            ID3D12DescriptorHeap*              buffer_heap, ID3D12DescriptorHeap*                      sampler_heap,
            const D3D12_GPU_DESCRIPTOR_HANDLE& buffer_handle, const D3D12_GPU_DESCRIPTOR_HANDLE&       sampler_handle,
            const UINT                         buffer_size, const UINT                                 sampler_size
        ) const
        {
            return;
        }
    };
}

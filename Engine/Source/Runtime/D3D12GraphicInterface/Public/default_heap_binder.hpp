#pragma once
#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include "CommandPair.h"
#include "Texture.h"
#include "TypeLibrary/Public/TypeLibrary.h"

namespace Engine
{
    struct default_heap_binder
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
            const CD3DX12_CPU_DESCRIPTOR_HANDLE cbv_handle(begin, g_cb_offset + slot, buffer_size);

            dev->CopyDescriptorsSimple(1, cbv_handle, cbv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        void SetShaderResource(
            ID3D12Device* dev, const D3D12_CPU_DESCRIPTOR_HANDLE& begin, const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle,
            UINT          slot, const UINT                        buffer_size
        ) const
        {
            const CD3DX12_CPU_DESCRIPTOR_HANDLE heap_handle(begin, g_srv_offset + slot, buffer_size);

            dev->CopyDescriptorsSimple(1, heap_handle, srv_handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        void SetShaderResources(
            ID3D12Device* dev, const D3D12_CPU_DESCRIPTOR_HANDLE& begin, UINT slot, UINT count,
            const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& data, const UINT buffer_size
        ) const
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE heap_handle(begin, g_srv_offset + slot, buffer_size);

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
            const CD3DX12_CPU_DESCRIPTOR_HANDLE uav_handle(begin, g_uav_offset + slot, buffer_size);

            dev->CopyDescriptorsSimple(1, uav_handle, uav, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        void BindGraphic(
            ID3D12RootSignature*               root_signature, const GraphicInterfaceContextPrimitive* context,
            ID3D12DescriptorHeap*              buffer_heap, ID3D12DescriptorHeap*                      sampler_heap,
            const D3D12_GPU_DESCRIPTOR_HANDLE& buffer_handle, const D3D12_GPU_DESCRIPTOR_HANDLE&       sampler_handle,
            const UINT                         buffer_size, const UINT                                 sampler_size
        ) const
        {
            const auto cmd_pair = static_cast<CommandPair*>(context->commandList);
            const auto cmd      = cmd_pair->GetList();

            cmd->SetGraphicsRootSignature(root_signature);

            ID3D12DescriptorHeap* heaps[]{buffer_heap, sampler_heap};

            cmd->SetDescriptorHeaps(2, heaps);

            cmd->SetGraphicsRootDescriptorTable(RASTERIZER_SLOT_SAMPLER, sampler_handle);

            cmd->SetGraphicsRootDescriptorTable(RASTERIZER_SLOT_SRV, buffer_handle);

            CD3DX12_GPU_DESCRIPTOR_HANDLE cb_handle(buffer_handle, g_cb_offset, buffer_size);

            cmd->SetGraphicsRootDescriptorTable(RASTERIZER_SLOT_CB, cb_handle);

            cb_handle.Offset(g_uav_offset - g_cb_offset, buffer_size);

            cmd->SetGraphicsRootDescriptorTable(RASTERIZER_SLOT_UAV, cb_handle);
        }

        void BindCompute(
            ID3D12RootSignature*               root_signature, const GraphicInterfaceContextPrimitive* context,
            ID3D12DescriptorHeap*              buffer_heap, ID3D12DescriptorHeap*                      sampler_heap,
            const D3D12_GPU_DESCRIPTOR_HANDLE& buffer_handle, const D3D12_GPU_DESCRIPTOR_HANDLE&       sampler_handle,
            const UINT                         buffer_size, const UINT                                 sampler_size
        ) const
        {
            const auto cmd_pair = static_cast<CommandPair*>(context->commandList);
            const auto cmd      = cmd_pair->GetList();

            cmd->SetComputeRootSignature(root_signature);

            ID3D12DescriptorHeap* heaps[]{buffer_heap, sampler_heap};

            cmd->SetDescriptorHeaps(2, heaps);

            cmd->SetComputeRootDescriptorTable(RASTERIZER_SLOT_SAMPLER, sampler_handle);

            cmd->SetComputeRootDescriptorTable(RASTERIZER_SLOT_SRV, buffer_handle);

            CD3DX12_GPU_DESCRIPTOR_HANDLE cb_handle(buffer_handle, g_cb_offset, buffer_size);

            cmd->SetComputeRootDescriptorTable(RASTERIZER_SLOT_CB, cb_handle);

            cb_handle.Offset(g_uav_offset - g_cb_offset, buffer_size);

            cmd->SetComputeRootDescriptorTable(RASTERIZER_SLOT_UAV, cb_handle);
        }
    };
}

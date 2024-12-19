#pragma once
#include "D3D12GraphicInterface.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "ThrowIfFailed.h"

#include "Source/Runtime/Core/SIMDExtension/Public/SIMDExtension.hpp"

namespace Engine
{
    template <size_t Alignment, D3D12_HEAP_TYPE HeapProperty, D3D12_RESOURCE_FLAGS Flags, D3D12_RESOURCE_STATES ResourceState>
    class D3D12GraphicMemoryPool : public GraphicMemoryPool
    {
    public:
        void Map(const void* src_data, const size_t count, const size_t stride) override
        {
            char*      data          = nullptr;
            const auto src_char_cast = static_cast<const char*>(src_data);
            const auto& resource = GetResource<ID3D12Resource>();

            DX::ThrowIfFailed(resource->Map(0, nullptr, reinterpret_cast<void**>(&data)));
            for (size_t i = 0; i < count; ++i)
            {
                SIMDExtension::_mm256_memcpy(data + (i * Align(stride, Alignment)), src_char_cast + (i * stride), stride);
            }
            resource->Unmap(0, nullptr);
        }

        [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const
        {
            return GetResource<ID3D12Resource>()->GetGPUVirtualAddress();
        }

    private:
        void InitializeBuffer(const size_t count, const size_t stride) override
        {
            const auto& heap_desc = CD3DX12_HEAP_PROPERTIES(HeapProperty);
            const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(Align(count * stride, Alignment), Flags);
            const auto& dev = static_cast<ID3D12Device2*>(g_graphic_interface.GetInterface().GetNativeInterface());

            DX::ThrowIfFailed
                (
                 dev->CreateCommittedResource
                 (
                  &heap_desc,
                  D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
                  &buffer_desc,
                  ResourceState,
                  nullptr,
                  IID_PPV_ARGS(GetAddressOf<ID3D12Resource>())
                 )
                );
        }
    };
}


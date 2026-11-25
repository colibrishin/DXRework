#pragma once
#include "D3D12GraphicInterface.h"
#include "D3D12GraphicResourcePrimitive.h"
#include "FrameRelease.h"

#include "Allocator.h"
#include "ThrowIfFailed.h"

#include "SIMDExtension.hpp"

namespace Engine
{
    template <size_t Alignment, D3D12_HEAP_TYPE HeapProperty, D3D12_RESOURCE_FLAGS Flags, D3D12_RESOURCE_STATES ResourceState>
    class D3D12GraphicMemoryPool : public GraphicMemoryPool
    {
    public:
        ~D3D12GraphicMemoryPool() override
        {
            KeepUntilFrameDone(std::move(m_resource_)); // lists of this frame may still read it
            for (Unique<IGraphicResource>& other : m_other_slots_)
            {
                KeepUntilFrameDone(std::move(other));
            }
        }
        void Map(const void* src_data, const size_t count, const size_t stride) override
        {
            if constexpr (HeapProperty == D3D12_HEAP_TYPE_UPLOAD)
            {
                SelectFrameSlot(count, stride); // the GPU reads upload memory directly: never write a frame's copy in flight
            }
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
        // Upload heaps: one resource per frame in flight; m_resource_ is the one of the frame being recorded.
        void SelectFrameSlot(const size_t count, const size_t stride)
        {
            const uint64_t frame = RecordingFrame();
            const size_t   slot  = frame % g_max_frames_in_flight;
            if (slot != m_slot_)
            {
                m_other_slots_[m_slot_] = std::move(m_resource_);
                m_slot_counts_[m_slot_] = m_count_;
                m_resource_             = std::move(m_other_slots_[slot]);
                m_count_                = m_slot_counts_[slot];
                m_slot_                 = slot;
            }
            const bool rewritten = m_slot_frames_[slot] == frame;
            CountFrameUpload(rewritten,
                             m_slot_frames_[slot] != UINT64_MAX && !rewritten && !FrameDone(m_slot_frames_[slot]), L"GraphicMemoryPool");
            m_slot_frames_[slot] = frame;
            // Written again in the same frame: a list recorded earlier may still read the first write.
            if (!m_resource_ || m_count_ < count || rewritten)
            {
                InitializeBuffer((std::max)(count, m_count_), stride); // defers the old resource
            }
        }

        void InitializeBuffer(const size_t count, const size_t stride) override
        {
            m_count_ = count;
            if (m_resource_)
            {
                KeepUntilFrameDone(std::move(m_resource_)); // growing: lists of this frame may still read the old one
            }
            m_resource_ = std::make_unique<D3D12GraphicResourcePrimitive>();
            
            const auto& heap_desc = CD3DX12_HEAP_PROPERTIES(HeapProperty);
            const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(Align(count * stride, Alignment), Flags);
            const auto& dev = static_cast<ID3D12Device2*>(g_graphic_accessor.GetInterface().GetNativeInterface());

            DX::ThrowIfFailed( dev->CreateCommittedResource( &heap_desc,
                                                             D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
                                                             &buffer_desc,
                                                             ResourceState,
                                                             nullptr,
                                                             IID_PPV_ARGS( GetAddressOf<ID3D12Resource>() ) ) );
        }

        size_t                                                          m_slot_  = 0;
        size_t                                                          m_count_ = 0;
        std::array<Unique<IGraphicResource>, g_max_frames_in_flight>    m_other_slots_{};
        std::array<size_t, g_max_frames_in_flight>                      m_slot_counts_{};
        std::array<uint64_t, g_max_frames_in_flight>                    m_slot_frames_{ UINT64_MAX, UINT64_MAX };
    };
}


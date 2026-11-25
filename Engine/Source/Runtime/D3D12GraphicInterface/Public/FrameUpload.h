#pragma once
#include <array>
#include <cstdint>

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <wrl/client.h>

#include "FrameRelease.h"
#include "ThrowIfFailed.h"

namespace Engine
{
    // One upload buffer per frame in flight for data the CPU writes every frame and a list copies (or the GPU reads)
    // later: the frame being recorded never writes a buffer a frame still on the GPU reads. Get() reports writing a
    // slot whose frame has not finished, and writing one slot twice in a frame, to CountFrameUpload.
    class FrameUploadSlots
    {
    public:
        // The buffer of the frame being recorded, at least bytes large (made on first use, replaced when smaller).
        ID3D12Resource* Get(ID3D12Device* dev, const UINT64 bytes, const D3D12_RESOURCE_STATES state, const wchar_t* name)
        {
            const uint64_t frame = RecordingFrame();
            const size_t   slot  = frame % g_max_frames_in_flight;
            CountFrameUpload( m_frames_[ slot ] == frame, m_frames_[ slot ] != s_never && m_frames_[ slot ] != frame &&
                                                          !FrameDone( m_frames_[ slot ] ) );
            m_frames_[ slot ] = frame;

            if ( !m_buffers_[ slot ] || m_sizes_[ slot ] < bytes )
            {
                KeepUntilFrameDone( std::move( m_buffers_[ slot ] ) );
                const auto heap = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );
                const auto desc = CD3DX12_RESOURCE_DESC::Buffer( bytes );
                DX::ThrowIfFailed( dev->CreateCommittedResource( &heap, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &desc, state,
                                                                 nullptr,
                                                                 IID_PPV_ARGS( m_buffers_[ slot ].ReleaseAndGetAddressOf() ) ) );
                DX::ThrowIfFailed( m_buffers_[ slot ]->SetName( name ) );
                m_sizes_[ slot ] = bytes;
            }
            return m_buffers_[ slot ].Get();
        }

        void Release()
        {
            for ( size_t i = 0; i < g_max_frames_in_flight; ++i )
            {
                KeepUntilFrameDone( std::move( m_buffers_[ i ] ) );
                m_sizes_[ i ]  = 0;
                m_frames_[ i ] = s_never;
            }
        }

        ~FrameUploadSlots()
        {
            Release();
        }

    private:
        static constexpr uint64_t s_never = UINT64_MAX;

        std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, g_max_frames_in_flight> m_buffers_{};
        std::array<UINT64, g_max_frames_in_flight>                                 m_sizes_{};
        std::array<uint64_t, g_max_frames_in_flight> m_frames_{ s_never, s_never };
    };
}

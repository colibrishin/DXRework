#pragma once

#include <directx/d3d12.h>
#include <wrl/client.h>

#include "Mesh.h"

namespace Engine 
{
    struct ENGINE_D3D12GRAPHICINTERFACE_API D3D12PrimitiveMesh : public IMesh
    {
        ~D3D12PrimitiveMesh() override;
        D3D12PrimitiveMesh() = default;
        void Generate( Resources::Mesh* mesh ) override;

        [[nodiscard]] uint64_t GetNativeVertexBufferGPUAddress() const override;
        [[nodiscard]] uint64_t GetNativeIndexBufferGPUAddress() const override;

    protected:
        const void* GetNativeVertexBufferInternal() const override;
        const void* GetNativeIndexBufferInternal() const override;

    private:
        ComPtr<ID3D12Resource>   m_native_vertex_buffer_;
        D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view_{};
        ComPtr<ID3D12Resource>   m_native_index_buffer_;
        D3D12_INDEX_BUFFER_VIEW  m_index_buffer_view_{};
        ComPtr<ID3D12Resource>   m_native_vertex_upload_buffer_;
        ComPtr<ID3D12Resource>   m_native_index_upload_buffer_;

#if CFG_RAYTRACING
        ComPtr<ID3D12Resource> m_raytracing_vertex_buffer_;
        ComPtr<ID3D12Resource> m_raytracing_index_buffer_;
        ComPtr<ID3D12Resource> m_raytracing_vertex_buffer_upload_;
        ComPtr<ID3D12Resource> m_raytracing_index_buffer_upload_;
#endif
    };
}

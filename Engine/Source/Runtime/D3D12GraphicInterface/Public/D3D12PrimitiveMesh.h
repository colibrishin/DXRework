#pragma once

#include <directx/d3d12.h>
#include <wrl/client.h>

#include "Source/Runtime/Resources/Mesh/Public/Mesh.h"

namespace Engine 
{
	struct D3D12GRAPHICINTERFACE_API D3D12PrimitiveMesh : public PrimitiveMesh
	{
		D3D12PrimitiveMesh() = default;
		void Generate(const Resources::Mesh* mesh) override;

	private:
		void SetNativeIndexBuffer(void* buffer) override;
		void SetNativeVertexBuffer(void* buffer) override;
		
		ComPtr<ID3D12Resource> m_native_vertex_buffer_;
		D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view_{};
		ComPtr<ID3D12Resource> m_native_index_buffer_;
		D3D12_INDEX_BUFFER_VIEW m_index_buffer_view_{};

		ComPtr<ID3D12Resource> m_native_vertex_upload_buffer_;
		ComPtr<ID3D12Resource> m_native_index_upload_buffer_;

#if CFG_RAYTRACING
		ComPtr<ID3D12Resource> m_raytracing_vertex_buffer_;
		ComPtr<ID3D12Resource> m_raytracing_index_buffer_;
		ComPtr<ID3D12Resource> m_raytracing_vertex_buffer_upload_;
		ComPtr<ID3D12Resource> m_raytracing_index_buffer_upload_;
#endif
	};
}

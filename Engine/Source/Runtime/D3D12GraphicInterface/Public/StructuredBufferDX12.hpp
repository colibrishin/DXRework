#pragma once
#include <directx/d3d12.h>
#include <wrl/client.h>

#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine::Graphics
{
	class ENGINE_D3D12GRAPHICINTERFACE_API D3D12StructuredBufferTypeless : public StructuredBufferTypeless 
	{
	public:
		D3D12StructuredBufferTypeless() = default;
		~D3D12StructuredBufferTypeless() override = default;

		void Clear() override;

		void TransitionToSRV(const GraphicInterfaceContextPrimitive* context) override;
		void TransitionToUAV(const GraphicInterfaceContextPrimitive* context) override;
		void TransitionCommon(const GraphicInterfaceContextPrimitive* context) override;

		[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;

	protected:
		void Create(const GraphicInterfaceContextPrimitive* context, UINT size, const void* initial_data, const size_t stride, const bool uav) override;
		void SetData(const GraphicInterfaceContextPrimitive* context, UINT size, const void* src_ptr, const size_t stride, const bool uav) override;
		void SetDataContainer(const GraphicInterfaceContextPrimitive* context, UINT size, const void* const* src_ptr, const size_t stride) override;
		void SetDataPointerContainer(const GraphicInterfaceContextPrimitive* context, UINT size, const void* const* src_ptr, const size_t stride) override;
		void GetData(const GraphicInterfaceContextPrimitive* context, UINT size, void* dst_ptr, const size_t stride) override;

		void CopySRVHeap(const GraphicInterfaceContextPrimitive* heap, const UINT slot) const override;
		void CopyUAVHeap(const GraphicInterfaceContextPrimitive* heap, const UINT slot) const override;

	private:
		void InitializeSRV(UINT size, const size_t stride);
		void InitializeUAV(UINT size, const size_t stride);
		void InitializeMainBuffer(UINT size, size_t stride);
		void InitializeUploadBuffer(const GraphicInterfaceContextPrimitive* context, UINT size, const void* initial_data, const size_t stride);
		void InitializeReadBuffer(UINT size, const size_t stride);

		D3D12_RESOURCE_STATES m_current_state_ = D3D12_RESOURCE_STATE_COMMON;

		ComPtr<ID3D12DescriptorHeap> m_srv_heap_{};
		ComPtr<ID3D12DescriptorHeap> m_uav_heap_{};

		ComPtr<ID3D12Resource> m_upload_buffer_{};
		ComPtr<ID3D12Resource> m_read_buffer_{};
		ComPtr<ID3D12Resource> m_buffer_{};

		UINT m_size_{};
		bool m_uav_ = false;
	};
}

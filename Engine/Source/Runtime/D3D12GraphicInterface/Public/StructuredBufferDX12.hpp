#pragma once
#include <directx/d3d12.h>
#include <wrl/client.h>

#include "Source/Runtime/Core/StructuredBuffer.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine::Graphics
{
	class D3D12GRAPHICINTERFACE_API D3D12StructuredBufferTypeless : public StructuredBufferTypelessBase 
	{
		D3D12StructuredBufferTypeless() = default;
		~D3D12StructuredBufferTypeless() override = default;

		void Clear() override;

		void TransitionToSRV(const GraphicInterfaceContextPrimitive* context) override;
		void TransitionToUAV(const GraphicInterfaceContextPrimitive* context) override;
		void TransitionCommon(const GraphicInterfaceContextPrimitive* context) override;

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;

	protected:
		void Create(const GraphicInterfaceContextPrimitive* context, UINT size, const void* initial_data, const size_t stride, const bool uav) override;
		void SetData(const GraphicInterfaceContextPrimitive* context, UINT size, const void* src_ptr, const size_t stride) override;
		void SetDataContainer(const GraphicInterfaceContextPrimitive* context, UINT size, const void* const* src_ptr, const size_t stride) override;
		void GetData(const GraphicInterfaceContextPrimitive* context, UINT size, void* dst_ptr, const size_t stride) override;

		void CopySRVHeap(const GraphicInterfaceContextPrimitive* heap, const UINT slot) const;
		void CopyUAVHeap(const GraphicInterfaceContextPrimitive* heap, const UINT slot) const;

	private:
		void InitializeSRV(UINT size, const size_t stride);
		void InitializeUAV(UINT size, const size_t stride);
		void InitializeMainBuffer(UINT size, size_t stride);
		void InitializeUploadBuffer(const GraphicInterfaceContextPrimitive* context, UINT size, const void* initial_data, const size_t stride);
		void InitializeReadBuffer(UINT size, const size_t stride);

		D3D12_RESOURCE_STATES m_current_state_ = D3D12_RESOURCE_STATE_COMMON;

		ComPtr<ID3D12DescriptorHeap> m_srv_heap_;
		ComPtr<ID3D12DescriptorHeap> m_uav_heap_;

		ComPtr<ID3D12Resource> m_upload_buffer_;
		ComPtr<ID3D12Resource> m_read_buffer_;
		ComPtr<ID3D12Resource> m_buffer_;

		UINT m_size_{};
		bool m_uav_ = false;
	};

	template <typename T>
	class D3D12StructuredBuffer : public D3D12StructuredBufferTypeless, public IStructuredBufferType<T>
	{
	public:
		D3D12StructuredBuffer() = default;
		~D3D12StructuredBuffer() override = default;

		void Create(const GraphicInterfaceContextPrimitive* context, UINT size, const T* initial_data, const bool uav) override
		{
			Create(context, size, initial_data, sizeof(T), uav);
		}

		void SetData(const GraphicInterfaceContextPrimitive* context, UINT size, const T* src_ptr) override 
		{
			Create(context, size, src_ptr, sizeof(T));
		}

		void SetDataContainer(const GraphicInterfaceContextPrimitive* context, UINT size, const T* const* src_ptr) override
		{
			Create(context, size, src_ptr, sizeof(T));
		}

		void GetData(const GraphicInterfaceContextPrimitive* context, UINT size, T* dst_ptr) override
		{
			GetData(context, size, dst_ptr, sizeof(T));
		}

		void CopySRVHeap(const GraphicInterfaceContextPrimitive* heap) const override 
		{
			if constexpr (is_client_sb<T>::value == true)
			{
				CopySRVHeap(heap, which_client_sb<T>::value);
			}
			else if constexpr (is_sb<T>::value == true)
			{
				CopySRVHeap(heap, which_sb<T>::value);
			}
		}

		void CopyUAVHeap(const GraphicInterfaceContextPrimitive* heap) const override
		{
			if constexpr (is_client_uav_sb<T>::value == true)
			{
				CopySRVHeap(heap, which_client_sb_uav<T>::value);
			}
			else if constexpr (is_uav_sb<T>::value == true)
			{
				CopySRVHeap(heap, which_sb_uav<T>::value);
			}
		}
	};
}

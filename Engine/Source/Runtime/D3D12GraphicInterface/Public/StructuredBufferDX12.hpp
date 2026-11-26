#pragma once
#include "IGraphicAPI.h"
#include <directx/d3d12.h>
#include <wrl/client.h>

#include "TypeLibrary.h"
#include "FrameUpload.h"

namespace Engine::Graphics
{
	class ENGINE_D3D12GRAPHICINTERFACE_API D3D12StructuredBufferTypeless : public IStructuredBuffer 
	{
	public:
		D3D12StructuredBufferTypeless() = default;
		~D3D12StructuredBufferTypeless() override;

		void Clear() override;

		void TransitionToSRV(const IGraphicContext* context) override;
		void TransitionToUAV(const IGraphicContext* context) override;
		void TransitionCommon(const IGraphicContext* context) override;

		[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;

	protected:
		void Create(const IGraphicContext* context, UINT size, const void* initial_data, const size_t stride, const bool uav) override;
		void SetData(const IGraphicContext* context, UINT size, const void* src_ptr, const size_t stride, const bool uav) override;
		void SetDataContainer(const IGraphicContext* context, UINT size, const void* const* src_ptr, const size_t stride) override;
		void SetDataPointerContainer(const IGraphicContext* context, UINT size, const void* const* src_ptr, const size_t stride) override;
		void GetData(const IGraphicContext* context, UINT size, void* dst_ptr, const size_t stride) override;
		bool RequestData(const IGraphicContext* context, UINT size, size_t stride, uint64_t tag) override;
		uint64_t ReadyData() override;
		void ReadData(uint64_t tag, UINT size, void* dst_ptr, size_t stride) override;

		void CopySRVHeap(const IGraphicContext* heap, const UINT slot) const override;
		void CopyUAVHeap(const IGraphicContext* heap, const UINT slot) const override;

	private:
		void InitializeSRV(UINT size, const size_t stride);
		void InitializeUAV(UINT size, const size_t stride);
		void InitializeMainBuffer(UINT size, size_t stride);
		void InitializeUploadBuffer(const IGraphicContext* context, UINT size, const void* initial_data, const size_t stride);
		void InitializeReadBuffer(UINT size, const size_t stride);
		void ReleaseReadbacks();
		ID3D12Resource* UploadBuffer(UINT size, size_t stride); // of the frame being recorded

    public:
        [[nodiscard]] void* GetResource() const override;

    private:
        D3D12_RESOURCE_STATES m_current_state_ = D3D12_RESOURCE_STATE_COMMON;

		ComPtr<ID3D12DescriptorHeap> m_srv_heap_{};
		ComPtr<ID3D12DescriptorHeap> m_uav_heap_{};

		FrameUploadSlots       m_upload_;
		ComPtr<ID3D12Resource> m_read_buffer_{};

		// RequestData copies: one more than the frames in flight, so the oldest has finished when it is reused.
		struct Readback
		{
			ComPtr<ID3D12Resource> buffer;
			UINT64                 bytes  = 0; // size of the buffer
			UINT64                 copied = 0; // bytes the request copied
			uint64_t               frame  = 0;
			uint64_t               tag   = 0; // 0: free (taken or dropped)
		};
		std::array<Readback, g_max_frames_in_flight + 1> m_readbacks_{};
		size_t                                           m_next_readback_ = 0;
		size_t                                           m_ready_readback_ = SIZE_MAX;
		ComPtr<ID3D12Resource> m_buffer_{};

		UINT m_size_{};
		bool m_uav_ = false;
	};
}

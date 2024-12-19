#pragma once
#include "Source/Runtime/Core/GraphicInterface.h"
#include "CommandPair.h"

#include <Windows.h>
#include <wrl/client.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <dxgi1_5.h>

namespace Engine 
{
	struct D3D12GRAPHICINTERFACE_API D3D12GraphicResourcePrimitive : public GraphicResourcePrimitive
	{
	public:
		void SetResource(void* resource) override
		{
			GraphicResourcePrimitive::SetResource(resource);
			m_native_resource_ = resource;
		}

		ID3D12Resource** GetAddressOf()
		{
			return m_native_resource_.GetAddressOf();
		}
		
	private:
		ComPtr<ID3D12Resource> m_native_resource_;
	};
	
	struct D3D12GRAPHICINTERFACE_API D3D12GraphicInterface : public GraphicInterface
	{
	public:
		void Initialize() override;
		void Shutdown() override;
		void WaitForNextFrame() override;
		void Present() override;

		void* GetNativeInterface() override;
		void* GetNativePipeline() override;

		GraphicInterfaceContextReturnType GetNewContext(const int8_t type, bool heap_allocation, const std::wstring_view debug_name) override;
		Strong<CommandListBase> GetCommandList(const int8_t type, const std::wstring_view debug_name) override;
		Unique<GraphicHeapBase> GetHeap() override;

		void SetViewport(const GraphicInterfaceContextPrimitive* context, const Viewport& viewport) override;
		void SetDefaultPipeline(const GraphicInterfaceContextPrimitive* context) override;
		void Draw(const GraphicInterfaceContextPrimitive* context, Resources::Shape* shape, const UINT instance_count) override;
		void Draw(const GraphicInterfaceContextPrimitive* context, Resources::Mesh* mesh, const UINT instance_count) override;
		void Bind(const GraphicInterfaceContextPrimitive* context, Resources::Shader* shader) override;
		void Bind(const GraphicInterfaceContextPrimitive* context, Resources::Texture* tex, const eBindType bind_type, const UINT slot, const UINT offset) override;
		void Unbind(const GraphicInterfaceContextPrimitive* context, Resources::Texture* tex, const eBindType bind_type) override;
		void Clear(const GraphicInterfaceContextPrimitive* context, Resources::Texture* tex, const eBindType clear_type) override;
		void ClearRenderTarget();
		void CopyRenderTarget(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex) const;

	protected:
		Unique<StructuredBufferTypelessBase>&& GetNativeStructuredBuffer() override;
		
	private:
		void InitializeDevice();
		void InitializePipeline();
		void DetachCommandThread();
		float GetAspectRatio();

		ComPtr<ID3D12Device2> m_dev_;
		ComPtr<IDXGISwapChain4> m_swap_chain_ = nullptr;

		std::vector<ComPtr<ID3D12Resource>> m_render_targets_;
		ComPtr<ID3D12DescriptorHeap>        m_rtv_heap_;
		UINT                                m_rtv_heap_size_{};
		ComPtr<ID3D12Resource>       m_depth_stencil_;
		ComPtr<ID3D12DescriptorHeap> m_dsv_heap_;
		uint64_t m_frame_idx_ = 0;

		ComPtr<ID3D12RootSignature> m_pipeline_root_signature_;

		Strong<DescriptorHandler> m_heap_handler_;
		CommandPairTask m_command_pair_task_;
		std::thread m_command_task_thread_;

		UINT s_video_card_memory_ = 0;
		UINT s_refresh_rate_numerator_ = 0;
		UINT s_refresh_rate_denominator_ = 0;
		DXGI_ADAPTER_DESC s_video_card_desc_ = {};

		Matrix m_projection_matrix_{};
		Matrix m_ortho_matrix_{};

		Matrix GetProjectionMatrix() override;
		Matrix GetOrthogonalMatrix() override;
	};
}
#pragma once
#include "Source/Runtime/Core/GraphicInterface.h"
#include "CommandPair.h"

#include <wrl/client.h>
#include <directx/d3d12.h>
#include <dxgi1_5.h>

#include "Source/Runtime/Core/ModuleManager/Public/IModule.h"

#include "D3D12GraphicInterface.generated.h"

namespace Engine
{
	ECLASS(module)
	struct ENGINE_D3D12GRAPHICINTERFACE_API D3D12GraphicInterfaceModule : public IModule
	{
		GENERATE_BODY
		bool InitializeImpl() override;
		bool ShutdownImpl() override;
		bool DynamicLoadable() override;
		const std::vector<std::string>& LoadAfter() const override;
	};

	struct ENGINE_D3D12GRAPHICINTERFACE_API D3D12GraphicResourcePrimitive : public GraphicResourcePrimitive
	{
	public:
		void SetResource(void* resource) override
		{
			GraphicResourcePrimitive::SetResource(resource);
			m_native_resource_ = static_cast<ID3D12Resource*>(resource);
		}

		ID3D12Resource** GetAddressOf()
		{
			return m_native_resource_.GetAddressOf();
		}
		
	private:
		ComPtr<ID3D12Resource> m_native_resource_;
	};
	
	struct ENGINE_D3D12GRAPHICINTERFACE_API D3D12GraphicInterface : public GraphicInterface
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(D3D12GraphicInterface)

		void Initialize() override; 
		void Shutdown() override;
		void WaitForNextFrame() override;
		void Present() override;

		void* GetNativeInterface() override;
		void* GetNativePipeline() override;

		PrimitiveTexture*       GetNewPrimitiveTexture() override;
		PrimitiveMesh*          GetNewPrimitiveMesh() override;
		GraphicPrimitiveShader* GetNewGraphicPrimitiveShader() override;
		ComputePrimitiveShader* GetNewComputePrimitiveShader() override;

		GraphicInterfaceContextReturnType GetNewContext(const int8_t type, bool heap_allocation, const std::wstring_view debug_name) override;

		CommandPairTask& GetCommandTask();
		Strong<CommandListBase> GetCommandList(const int8_t type, const std::wstring_view debug_name) override;
		Unique<GraphicHeapBase> GetHeap() override;

		void SetViewport(const GraphicInterfaceContextPrimitive* context, const Viewport& viewport) override;
		void SetDefaultRenderTarget(const GraphicInterfaceContextPrimitive* context) override;
		void SetDefaultGraphicPipeline(const GraphicInterfaceContextPrimitive* context) override;
		void SetDefaultComputePipeline(const GraphicInterfaceContextPrimitive* context) override;

		void Draw(const GraphicInterfaceContextPrimitive* context, const Resources::Mesh* mesh, UINT instance_count, UINT instance_offset) override;
		void Dispatch(const GraphicInterfaceContextPrimitive* context, const Resources::ComputeShader* shader, const Graphics::SBs::LocalParamSB& local_param, const UINT group_count[3]) override;
		void BindGraphic(const GraphicInterfaceContextPrimitive* context, const Resources::Shader* shader) override;
		void BindCompute(const GraphicInterfaceContextPrimitive* context, const Resources::ComputeShader* shader) override;

		inline void Transit(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const D3D12_RESOURCE_STATES before, const D3D12_RESOURCE_STATES after);
		void TransitTo(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const eBindType bind_type) override;
		void TransitBack(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const eBindType bind_type) override;
		
		inline void TransitMultiple(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* texes, const size_t count, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
		void TransitToMultiple(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* texes, const size_t count, const eBindType bind_type) override;
		void TransitBackMultiple(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* texes, const size_t count, const eBindType bind_type) override;
		
		void Bind(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const eBindType bind_type, const UINT slot, const UINT offset) override;
		void BindMultiple(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* rtvs, const size_t rtv_count, Resources::Texture* dsv) override;
		void BindMultiple(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* textures, const eBindType bind_type, const UINT slot, const UINT offset, const size_t count) override;

		void Clear(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const eBindType clear_type) override;
		void ClearRenderTarget() override;
		void CopyRenderTarget(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex) override;
		
		Matrix GetProjectionMatrix() override;
		Matrix GetOrthogonalMatrix() override;
		
	protected:
		StructuredBufferTypeless* GetNativeStructuredBuffer() override;
		ConstantBufferTypeless* GetNativeConstantBuffer() override;
		
	private:
		void InitializeDevice();
		void InitializePipeline();
		void DetachCommandThread();
		float GetAspectRatio();
		
		StructuredBufferTypeProxy<Graphics::SBs::LocalParamSB> m_local_param_;
		
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
	};
}

#pragma once
#include "CommandPair.h"
#include "GraphicInterface.h"

#include <dxgi1_5.h>
#include <directx/d3d12.h>
#include <wrl/client.h>

#include "DescriptorHandler.hpp"
#include "default_heap_allocator.hpp"
#include "default_heap_binder.hpp"
#include "default_heap_getter.hpp"
#include "raytracing_heap_allocator.hpp"
#include "raytracing_heap_binder.hpp"
#include "raytracing_heap_getter.hpp"

#include "ModuleManager.h"

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

#if CFG_RAYTRACING
	struct ENGINE_D3D12GRAPHICINTERFACE_API D3D12GraphicInterface : public virtual GraphicInterface, public virtual RaytracingExtensionInterface
#else
	struct ENGINE_D3D12GRAPHICINTERFACE_API D3D12GraphicInterface : public GraphicInterface
#endif
	{
		INLINE_COMPILE_TIME_TYPENAME(D3D12GraphicInterface)
	public:
	    D3D12GraphicInterface();
		void Initialize() override;
		void Shutdown() override;
		void WaitForNextFrame() override;
		void Present() override;

		void* GetNativeInterface() override;
		void* GetNativePipeline() override;

		bool IsRaytracingSupported() override;
		void InitializeSampler();
		void InitializeRaytracing() override;
		void ShutdownRaytracing() override;

	    Unique<GraphicHeapBase> GetRaytracingHeap() override;
		
		void* GetRaytracingNativeInterface() override;
		void* GetRaytracingNativePipeline() override;

	    bool BuildTopLevelAccelerationBuffer(
            const GraphicInterfaceContextPrimitive* context,
            RenderMap const* render_map,
            size_t render_map_size,
            AccelStructBuffer& out_tlas_buffer,
            const ObjectPredication& predication = {}) override;
	    
		void DispatchRay(
            const GraphicInterfaceContextPrimitive* context, const Resources::RaytracingShader* shader, const
            StructuredBufferTypeProxy<Graphics::SBs::LightSB>& light, const StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>
            & instances, const ConstantBufferTypeProxy<Graphics::CBs::PerspectiveCB>& perspective, const ConstantBufferTypeProxy
            <Graphics::CBs::ParamCB>& param, const byte_stream& hit_records, const AccelStructBuffer& top_level_accel_buffer
        ) override;

	    void CopyRaytracingToRenderTarget(const GraphicInterfaceContextPrimitive* context) override;
	    
	private:
		void QueryDevice();
		void InitializeRaytracingDescriptorHeaps();
		void InitializeGlobalRootSignature();
		void InitializeOutputBuffer();
		
		ComPtr<ID3D12Device5> m_raytracing_dev_ = nullptr;
		ComPtr<ID3D12RootSignature> m_raytracing_root_pipeline_ = nullptr;

	    Strong<DescriptorHandler<raytracing_heap_allocator, raytracing_heap_getter, raytracing_heap_binder>> m_raytracing_heap_handler_{};

	    ComPtr<ID3D12DescriptorHeap> m_raytracing_sampler_heap_ = nullptr;
		ComPtr<ID3D12Resource> m_output_buffer_ = nullptr;
	    ComPtr<ID3D12DescriptorHeap> m_output_heap_ = nullptr;

	public:
#endif
		PrimitiveTexture*       GetNewPrimitiveTexture() override;
		PrimitiveMesh*          GetNewPrimitiveMesh() override;
		GraphicPrimitiveShader* GetNewGraphicPrimitiveShader() override;
		ComputePrimitiveShader* GetNewComputePrimitiveShader() override;
	    PrimitiveFont          *GetNewPrimitiveFont() override;
	    PrimitiveSampler       *GetNewPrimitiveSampler() override;
	    RaytracingPrimitiveShader* GetNewRaytracingShader() override;

		GraphicInterfaceContextReturnType GetNewContext(const int8_t type, bool heap_allocation, const std::wstring_view debug_name) override;

		CommandPairTask        &GetCommandTask();
        Strong<CommandListBase> GetCommandList( const int8_t type, const std::wstring_view debug_name ) override;
        Unique<GraphicHeapBase> GetHeap() override;

		void SetViewport( const GraphicInterfaceContextPrimitive *context, const Viewport &viewport ) override;
        void SetDefaultRenderTarget( const GraphicInterfaceContextPrimitive *context ) override;
        void SetDefaultGraphicPipeline( const GraphicInterfaceContextPrimitive *context ) override;
        void SetDefaultComputePipeline( const GraphicInterfaceContextPrimitive *context ) override;

		void Draw( const GraphicInterfaceContextPrimitive *context,
                   const Resources::Mesh                  *mesh,
                   UINT                                    instance_count,
                   UINT                                    instance_offset ) override;
        void Dispatch( const GraphicInterfaceContextPrimitive *context,
                       const Resources::ComputeShader         *shader,
                       const Graphics::SBs::LocalParamSB      &local_param,
                       const UINT                              group_count[ 3 ] ) override;
        void BindGraphic( const GraphicInterfaceContextPrimitive *context, const Resources::Shader *shader ) override;
        void BindCompute( const GraphicInterfaceContextPrimitive *context,
                          const Resources::ComputeShader         *shader ) override;

		inline void Transit( const GraphicInterfaceContextPrimitive *context,
                             const Resources::Texture               *tex,
                             const D3D12_RESOURCE_STATES             before,
                             const D3D12_RESOURCE_STATES             after );
        void        TransitTo( const GraphicInterfaceContextPrimitive *context,
                               const Resources::Texture               *tex,
                               const eBindType                         bind_type ) override;
        void        TransitBack( const GraphicInterfaceContextPrimitive *context,
                                 const Resources::Texture               *tex,
                                 const eBindType                         bind_type ) override;

        inline void TransitMultiple( const GraphicInterfaceContextPrimitive *context,
                                     const Resources::Texture *const        *texes,
                                     const size_t                            count,
                                     D3D12_RESOURCE_STATES                   before,
                                     D3D12_RESOURCE_STATES                   after );
        void        TransitToMultiple( const GraphicInterfaceContextPrimitive *context,
                                       const Resources::Texture *const        *texes,
                                       const size_t                            count,
                                       const eBindType                         bind_type ) override;
        void        TransitBackMultiple( const GraphicInterfaceContextPrimitive *context,
                                         const Resources::Texture *const        *texes,
                                         const size_t                            count,
                                         const eBindType                         bind_type ) override;

        void Bind( const GraphicInterfaceContextPrimitive *context,
                   const Resources::Texture               *tex,
                   const eBindType                         bind_type,
                   const UINT                              slot,
                   const UINT                              offset ) override;
        void BindMultiple( const GraphicInterfaceContextPrimitive *context,
                           const Resources::Texture *const        *rtvs,
                           const size_t                            rtv_count,
                           Resources::Texture                     *dsv ) override;
        void BindMultiple( const GraphicInterfaceContextPrimitive *context,
                           const Resources::Texture *const        *textures,
                           const eBindType                         bind_type,
                           const UINT                              slot,
                           const UINT                              offset,
                           const size_t                            count ) override;

        void Clear( const GraphicInterfaceContextPrimitive *context,
                    const Resources::Texture               *tex,
                    const eBindType                         clear_type ) override;
        void ClearRenderTarget() override;
        void CopyRenderTarget( const GraphicInterfaceContextPrimitive *context,
                               const Resources::Texture               *tex ) override;

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

        ComPtr<ID3D12Device2>   m_dev_;
        ComPtr<IDXGISwapChain4> m_swap_chain_ = nullptr;

        std::vector<ComPtr<ID3D12Resource>> m_render_targets_;
        ComPtr<ID3D12DescriptorHeap>        m_rtv_heap_;
        UINT                                m_rtv_heap_size_{};
        ComPtr<ID3D12Resource>              m_depth_stencil_;
        ComPtr<ID3D12DescriptorHeap>        m_dsv_heap_;
        uint64_t                            m_frame_idx_ = 0;

        ComPtr<ID3D12RootSignature> m_pipeline_root_signature_;

        Strong<DescriptorHandler<default_heap_allocator, default_heap_getter, default_heap_binder>> m_heap_handler_;
        CommandPairTask           m_command_pair_task_;
        std::thread               m_command_task_thread_;

        UINT              s_video_card_memory_        = 0;
        UINT              s_refresh_rate_numerator_   = 0;
        UINT              s_refresh_rate_denominator_ = 0;
        DXGI_ADAPTER_DESC s_video_card_desc_          = {};

        Matrix m_projection_matrix_{};
        Matrix m_ortho_matrix_{};
	};
}

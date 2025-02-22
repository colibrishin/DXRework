#include "D3D12GraphicInterface.h"
#include "D3D12GraphicInterface.generated.h"

#include <dxgidebug.h>

#include "D3D12GraphicPrimitiveShader.h"
#include "D3D12PrimitiveMesh.h"
#include "D3D12PrimitiveFont.h"
#include "D3D12PrimitiveSampler.h"
#include "ThrowIfFailed.h"

#include "StructuredBufferDX12.hpp"
#include "Source/Runtime/Managers/WinAPIWrapper/Public/WinAPIWrapper.hpp"
#include "Source/Runtime/Resources/ComputeShader/Public/ComputeShader.h"
#include "Source/Runtime/Resources/Mesh/Public/Mesh.h"
#include "Source/Runtime/Resources/Shape/Public/Shape.h"
#include "Source/Runtime/Resources/Texture/Public/Texture.h"

#include "D3D12PrimitiveTexture.h"
#include "D3D12ComputePrimitiveShader.h"
#include "D3D12ConstantBuffer.hpp"
#include "D3D12GraphicMemoryPool.h"
#include "D3D12GraphicResourcePrimitive.h"
#include "ToolkitAPI.h"

#if CFG_RAYTRACING
#include "RaytracingShader.h"
#include "D3D12RaytracingShader.h"
#endif

#include "CoreModule/Public/CoreModule.h"

MODULE_IMPL(Engine::D3D12GraphicInterfaceModule, D3D12GraphicInterface)

bool Engine::D3D12GraphicInterfaceModule::InitializeImpl()
{
	GraphicInterfaceAccessor::SetGraphicInterface<D3D12GraphicInterface>();
	
	CoreModule::GetContext().AddManager(
		CoreLoop::LOOP_TYPE_RENDER,
		Managers::ToolkitAPI::GetInstance);

	return true;
}

bool Engine::D3D12GraphicInterfaceModule::ShutdownImpl()
{
	CoreModule::GetContext().RemoveManager(
		CoreLoop::LOOP_TYPE_RENDER,
		Managers::ToolkitAPI::GetInstance);

	auto& gi = GraphicInterfaceAccessor::GetInterface();
	gi.Shutdown();

	return true;
}

bool Engine::D3D12GraphicInterfaceModule::DynamicLoadable()
{
	return true;
}
const std::vector<std::string>& Engine::D3D12GraphicInterfaceModule::LoadAfter() const
{
#if Platform == Windows
	static std::vector<std::string> load_after{ "WinAPIWrapper" };
#endif
	return load_after;
}


Engine::D3D12GraphicInterface::D3D12GraphicInterface() {}

void Engine::D3D12GraphicInterface::Initialize()
{
	if (!WinAPI::WinAPIWrapper::GetHWND())
	{
		throw std::runtime_error("WinAPI does not initialized!");
	}

	m_projection_matrix_ = DirectX::XMMatrixPerspectiveFovLH
	(
		CFG_FOV, GetAspectRatio(),
		CFG_SCREEN_NEAR, CFG_SCREEN_FAR
	);
	m_ortho_matrix_ = DirectX::XMMatrixOrthographicLH
	(
		static_cast<float>(CFG_WIDTH),
		static_cast<float>(CFG_HEIGHT),
		CFG_SCREEN_NEAR, CFG_SCREEN_FAR
	);

	InitializeDevice();
	DetachCommandThread();

#if CFG_RAYTRACING
    if (auto& rgi = static_cast<RaytracingExtensionInterface&>(*this);
        rgi.IsRaytracingSupported())
    {
        rgi.InitializeRaytracing();
    }
#endif
}

void Engine::D3D12GraphicInterface::Shutdown()
{
	m_command_pair_task_.StopTask();

#if WITH_DEBUG
	HMODULE hModule = GetModuleHandleW(L"dxgidebug.dll");
	auto    DXGIGetDebugInterfaceFunc =
		reinterpret_cast<decltype(DXGIGetDebugInterface)*>(
			GetProcAddress(hModule, "DXGIGetDebugInterface"));

	IDXGIDebug* pDXGIDebug;
	DXGIGetDebugInterfaceFunc(IID_PPV_ARGS(&pDXGIDebug));
	pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL);
#endif // WITH_DEBUG
}

void Engine::D3D12GraphicInterface::WaitForNextFrame()
{
	if (WaitForSingleObjectEx
	(
		m_swap_chain_->GetFrameLatencyWaitableObject(), CFG_FRAME_LATENCY_TOLERANCE_SECOND * 1000,
		true
	) != WAIT_OBJECT_0)
	{
#if WITH_DEBUG
		OutputDebugString(TEXT("Waiting for Swap chain had an issue."));
#endif
	}

	const uint64_t next_buffer = m_swap_chain_->GetCurrentBackBufferIndex();
	m_command_pair_task_.SwapBuffer(next_buffer);
	m_frame_idx_ = next_buffer;
}

void Engine::D3D12GraphicInterface::Present()
{
	const auto& cmd = m_command_pair_task_.Acquire(D3D12_COMMAND_LIST_TYPE_DIRECT, false, L"Finalize Render").lock();

	const auto present_barrier = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_render_targets_[m_frame_idx_].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	cmd->SoftReset();
	cmd->GetList()->ResourceBarrier(1, &present_barrier);
	cmd->Execute();

	DXGI_PRESENT_PARAMETERS params;
	params.DirtyRectsCount = 0;
	params.pDirtyRects = nullptr;
	params.pScrollRect = nullptr;
	params.pScrollOffset = nullptr;

	DX::ThrowIfFailed
	(
		m_swap_chain_->Present1
		(
			CFG_VSYNC ? 1 : 0, DXGI_PRESENT_DO_NOT_WAIT,
			&params
		)
	);
}

void* Engine::D3D12GraphicInterface::GetNativeInterface()
{
	return m_dev_.Get();
}

void* Engine::D3D12GraphicInterface::GetNativePipeline()
{
	return m_pipeline_root_signature_.Get();
}

#if CFG_RAYTRACING
bool Engine::D3D12GraphicInterface::IsRaytracingSupported()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	DX::ThrowIfFailed(m_dev_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
													&options5, sizeof(options5)));

	return options5.RaytracingTier > D3D12_RAYTRACING_TIER_1_0;
}

void Engine::D3D12GraphicInterface::InitializeRaytracing()
{
	if (IsRaytracingSupported())
	{
		QueryDevice();
		InitializeGlobalRootSignature();
	    InitializeRaytracingDescriptorHeaps();
		InitializeOutputBuffer();
	}
}

void                                    Engine::D3D12GraphicInterface::ShutdownRaytracing() { }

Engine::Unique<Engine::GraphicHeapBase> Engine::D3D12GraphicInterface::GetRaytracingHeap()
{
    return m_raytracing_heap_handler_->Acquire();
}

void* Engine::D3D12GraphicInterface::GetRaytracingNativeInterface()
{
	return m_raytracing_dev_.Get();
}

void* Engine::D3D12GraphicInterface::GetRaytracingNativePipeline()
{
	return m_raytracing_root_pipeline_.Get();
}

bool Engine::D3D12GraphicInterface::BuildTopLevelAccelerationBuffer(
    const GraphicInterfaceContextPrimitive* context,
    RenderMap const* render_map,
    const size_t render_map_size,
    AccelStructBuffer& out_tlas_buffer,
    const ObjectPredication& predication)
{
    const auto& cmd = static_cast<CommandPair*>(context->commandList);
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instance_descs;
    concurrent_fast_pool_map<Resources::ShaderBase*, size_t> hit_group_id;

    size_t shader_count = 0;
    size_t instance_count = 0;
    
    for (size_t map_idx = 0; map_idx < render_map_size; ++map_idx)
    {
        for (const auto& mesh_map : render_map[map_idx] | std::views::values)
        {
            for (const auto& [shader, shader_map] : mesh_map)
            {
                for (const auto& [mesh, instance] : shader_map)
                {
                    size_t current_hit_group;
                    
                    if (decltype(hit_group_id)::accessor acc;
                        hit_group_id.find(acc, shader.get()))
                    {
                        current_hit_group = acc->second;
                    }
                    else
                    {
                        hit_group_id.emplace(shader.get(), shader_count);
                        current_hit_group = shader_count;
                        ++shader_count;
                    }
                    
                    for (size_t i = 0; i < instance.size(); ++i)
                    {
                        if (predication && !predication(instance[i].object))
                        {
                            continue;
                        }
                        
                        D3D12_RAYTRACING_INSTANCE_DESC desc
                        {
                            .Transform = {},
                            .InstanceID = static_cast<UINT>(instance_count + i),
                            .InstanceMask = 1,
                            .InstanceContributionToHitGroupIndex = static_cast<UINT>(current_hit_group),
                            .Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE,
                            .AccelerationStructure = mesh->GetBLAS().resultPool->GetResource<ID3D12Resource>()->GetGPUVirtualAddress()
                        };

                        const auto& world = instance[i].instance->GetParam<Matrix>(0);
                        SIMDExtension::_mm256_memcpy(desc.Transform, &world, sizeof(desc.Transform));
                        instance_descs.push_back(desc);
                    }

                    instance_count += instance.size();
                }
            }
        }
    }

    if (!out_tlas_buffer.instanceDescPool)
    {
        out_tlas_buffer.instanceDescPool = std::make_unique<D3D12GraphicMemoryPool<D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ>>();
    }
    
    out_tlas_buffer.instanceDescPool->Update(instance_descs.data(), instance_descs.size(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlas_input
    {
        .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
        .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
        .NumDescs = static_cast<UINT>(instance_descs.size()),
        .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
        .InstanceDescs = out_tlas_buffer.instanceDescPool->GetResource<ID3D12Resource>()->GetGPUVirtualAddress()
    };

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info{};
    m_raytracing_dev_->GetRaytracingAccelerationStructurePrebuildInfo(&tlas_input, &prebuild_info);

    if (prebuild_info.ResultDataMaxSizeInBytes == 0)
    {
        return false;
    }

    const auto& result_size = Align(
        prebuild_info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

    const auto& scratch_size = Align(
        prebuild_info.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

    if (!out_tlas_buffer.scratchPool)
    {
        out_tlas_buffer.scratchPool = std::make_unique<D3D12GraphicMemoryPool<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS>>();
    }

    if (!out_tlas_buffer.resultPool)
    {
        out_tlas_buffer.resultPool = std::make_unique<D3D12GraphicMemoryPool<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE>>();
    }

    out_tlas_buffer.scratchPool->Update(nullptr, 1, scratch_size);
    out_tlas_buffer.resultPool->Update(nullptr, 1, result_size);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = {};

    build_desc.Inputs                           = tlas_input;
    build_desc.DestAccelerationStructureData    = out_tlas_buffer.resultPool->GetResource<ID3D12Resource>()->GetGPUVirtualAddress();
    build_desc.ScratchAccelerationStructureData = out_tlas_buffer.scratchPool->GetResource<ID3D12Resource>()->GetGPUVirtualAddress();

    cmd->GetList4()->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

    const auto& uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(out_tlas_buffer.resultPool->GetResource<ID3D12Resource>());
    cmd->GetList4()->ResourceBarrier(1, &uav_barrier);

    out_tlas_buffer.empty = false;

    return true;
}

void Engine::D3D12GraphicInterface::DispatchRay(
    const GraphicInterfaceContextPrimitive* context,
    const Resources::RaytracingShader* shader,
    const StructuredBufferTypeProxy<Graphics::SBs::LightSB>& light,
    const StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>& instances,
    const ConstantBufferTypeProxy<Graphics::CBs::PerspectiveCB>& perspective,
    const ConstantBufferTypeProxy<Graphics::CBs::ParamCB>& param,
    const byte_stream& hit_records,
    const AccelStructBuffer& top_level_accel_buffer)
{
    const auto& cmd = static_cast<CommandPair*>( context->commandList );

    cmd->GetList4()->SetComputeRootSignature( m_raytracing_root_pipeline_.Get() );

    if ( shader )
    {
        cmd->GetList4()->SetPipelineState1(static_cast<ID3D12StateObject*>(shader->GetPrimitive().GetNativeShader()));
    }

    perspective.Flush(context);
    param.Flush(context);

    ID3D12DescriptorHeap* heaps[] = { m_output_heap_.Get() };
    cmd->GetList4()->SetDescriptorHeaps(1, heaps);
    
    cmd->GetList4()->SetComputeRootShaderResourceView(
        RAYTRACING_GLOBAL_SLOT_TLAS,
        top_level_accel_buffer.resultPool->GetResource<ID3D12Resource>()->GetGPUVirtualAddress());

    cmd->GetList4()->SetComputeRootShaderResourceView(
        RAYTRACING_GLOBAL_SLOT_LIGHT,
        light.GetGPUAddress());
    
    cmd->GetList4()->SetComputeRootShaderResourceView(
        RAYTRACING_GLOBAL_SLOT_INSTANCE,
        instances.GetGPUAddress());
        
    cmd->GetList4()->SetComputeRootDescriptorTable(
        RAYTRACING_GLOBAL_SLOT_OUTPUT,
        m_output_heap_->GetGPUDescriptorHandleForHeapStart());
    
    cmd->GetList4()->SetComputeRootConstantBufferView(
        RAYTRACING_GLOBAL_SLOT_WVP,
        perspective.GetGPUAddress());

    cmd->GetList4()->SetComputeRootConstantBufferView(
        RAYTRACING_GLOBAL_SLOT_PARAM,
        param.GetGPUAddress());

    auto& shader_primitive = static_cast<RaytracingPrimitiveShader&>(shader->GetPrimitive());
    shader_primitive.UpdateShaderRecords(RAY_SHADER_REC_HIT, hit_records);
    
    D3D12_DISPATCH_RAYS_DESC ray_desc{};
    ray_desc.Width = CFG_WIDTH;
    ray_desc.Height = CFG_HEIGHT;
    ray_desc.Depth = 1;
    
    const auto& exported = shader->GetHasExport();
    const auto& record_sizes = shader->GetShaderRecordSizes();
    const bool hit_merged[RAY_SHADER_REC_MAX]
    {
        exported[RAY_SHADER_GEN],
        exported[RAY_SHADER_ANY_HIT] || exported[RAY_SHADER_CLOSEST_HIT],
        exported[RAY_SHADER_MISS]
    };

    for (size_t i = 0; i < std::size(hit_merged); ++i)
    {
        if (hit_merged[i])
        {
            switch (i)
            {
            case RAY_SHADER_REC_GEN:
                {
                    auto resource = static_cast<ID3D12Resource*>(shader_primitive.GetShaderRecord(RAY_SHADER_REC_GEN));
                    ray_desc.RayGenerationShaderRecord.StartAddress = resource->GetGPUVirtualAddress();
                    ray_desc.RayGenerationShaderRecord.SizeInBytes = record_sizes[i];
                    break;
                }
            case RAY_SHADER_REC_HIT:
                {
                    auto resource = static_cast<ID3D12Resource*>(shader_primitive.GetShaderRecord(RAY_SHADER_REC_HIT));
                    ray_desc.HitGroupTable.StartAddress = resource->GetGPUVirtualAddress();
                    ray_desc.HitGroupTable.SizeInBytes = hit_records.size();
                    ray_desc.HitGroupTable.StrideInBytes = record_sizes[i];
                    break;
                }
            case RAY_SHADER_REC_MISS:
                {
                    auto resource = static_cast<ID3D12Resource*>(shader_primitive.GetShaderRecord(RAY_SHADER_REC_MISS));
                    ray_desc.MissShaderTable.StartAddress = resource->GetGPUVirtualAddress();
                    ray_desc.MissShaderTable.SizeInBytes = record_sizes[i];
                    ray_desc.MissShaderTable.StrideInBytes = record_sizes[i];
                    break;
                }
            default:
                throw std::runtime_error("Unknown type iterated");
            }
        }
    }
    
    cmd->GetList4()->DispatchRays(&ray_desc);
}

void Engine::D3D12GraphicInterface::CopyRaytracingToRenderTarget(const GraphicInterfaceContextPrimitive* context)
{
    const auto& cmd = static_cast<CommandPair*>(context->commandList);
    
    const auto& copy_barrier = CD3DX12_RESOURCE_BARRIER::Transition
                (
                 m_output_buffer_.Get(),
                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                 D3D12_RESOURCE_STATE_COPY_SOURCE
                );

    const auto& dst_barrier = CD3DX12_RESOURCE_BARRIER::Transition
            (
             m_render_targets_[m_frame_idx_].Get(),
             D3D12_RESOURCE_STATE_RENDER_TARGET,
             D3D12_RESOURCE_STATE_COPY_DEST
            );

    cmd->GetList4()->ResourceBarrier(1, &copy_barrier);
    cmd->GetList4()->ResourceBarrier(1, &dst_barrier);

    cmd->GetList4()->CopyResource
            (
             m_render_targets_[m_frame_idx_].Get(),
             m_output_buffer_.Get()
            );

    const auto& uav_barrier = CD3DX12_RESOURCE_BARRIER::Transition
            (
             m_output_buffer_.Get(),
             D3D12_RESOURCE_STATE_COPY_SOURCE,
             D3D12_RESOURCE_STATE_UNORDERED_ACCESS
            );

    const auto& rtv_barrier = CD3DX12_RESOURCE_BARRIER::Transition
            (
             m_render_targets_[m_frame_idx_].Get(),
             D3D12_RESOURCE_STATE_COPY_DEST,
             D3D12_RESOURCE_STATE_RENDER_TARGET
            );

    cmd->GetList4()->ResourceBarrier(1, &uav_barrier);
    cmd->GetList4()->ResourceBarrier(1, &rtv_barrier);
}

void Engine::D3D12GraphicInterface::QueryDevice()
{
	DX::ThrowIfFailed(m_dev_->QueryInterface(IID_PPV_ARGS(m_raytracing_dev_.GetAddressOf())));
}

void Engine::D3D12GraphicInterface::InitializeRaytracingDescriptorHeaps()
{
    m_raytracing_heap_handler_ = boost::make_shared<decltype(m_raytracing_heap_handler_)::element_type>();
	m_raytracing_heap_handler_->Initialize(m_dev_.Get(), m_raytracing_root_pipeline_.Get());
}

void Engine::D3D12GraphicInterface::InitializeGlobalRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE1 range;
    range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 1);
    
	CD3DX12_ROOT_PARAMETER1 root_param[RAYTRACING_GLOBAL_SLOT_COUNT];
	root_param[RAYTRACING_GLOBAL_SLOT_TLAS].InitAsShaderResourceView( 0, 1 ); // TLAS Buffer
    root_param[RAYTRACING_GLOBAL_SLOT_LIGHT].InitAsShaderResourceView( 1, 1 ); // Light Buffer
    root_param[RAYTRACING_GLOBAL_SLOT_INSTANCE].InitAsShaderResourceView( 2, 1 ); // Instance Buffer
    root_param[RAYTRACING_GLOBAL_SLOT_OUTPUT].InitAsDescriptorTable( 1, &range ); // Output Buffer
    root_param[RAYTRACING_GLOBAL_SLOT_WVP].InitAsConstantBufferView( 0, 0 ); // WVP Buffer
    root_param[RAYTRACING_GLOBAL_SLOT_PARAM].InitAsConstantBufferView( 1, 0 ); // Param Buffer

    // Output sampler
    constexpr D3D12_STATIC_SAMPLER_DESC sampler_desc
    {
        .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        .MipLODBias = 0,
        .MaxAnisotropy = 0,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
        .MinLOD = 0,
        .MaxLOD = D3D12_FLOAT32_MAX,
        .ShaderRegister = 0,
        .RegisterSpace = 1,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
    };
    
	const auto& global_root_sign_desc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(
		std::size( root_param ),
		root_param,
		1,
		&sampler_desc,
		D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	DX::ThrowIfFailed( D3D12SerializeVersionedRootSignature(
		&global_root_sign_desc,
		signature.ReleaseAndGetAddressOf(),
		error.ReleaseAndGetAddressOf() ) );

	if (error) { OutputDebugStringA( static_cast<const char*>( error->GetBufferPointer() ) ); }

	DX::ThrowIfFailed( m_dev_->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS( m_raytracing_root_pipeline_.ReleaseAndGetAddressOf() ) ) );
}

void Engine::D3D12GraphicInterface::InitializeOutputBuffer()
{
    const auto& res_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, CFG_WIDTH, CFG_HEIGHT, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    DX::ThrowIfFailed(
         m_dev_->CreateCommittedResource(
          &default_heap,
          D3D12_HEAP_FLAG_NONE,
          &res_desc,
          D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
          nullptr,
          IID_PPV_ARGS( m_output_buffer_.ReleaseAndGetAddressOf() ) ) );

    constexpr D3D12_DESCRIPTOR_HEAP_DESC desc
    {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = 1,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask = 0
    };

    DX::ThrowIfFailed( m_dev_->CreateDescriptorHeap( &desc, IID_PPV_ARGS( m_output_heap_.GetAddressOf() ) ) );

    m_dev_->CreateUnorderedAccessView( m_output_buffer_.Get(), nullptr, nullptr, m_output_heap_->GetCPUDescriptorHandleForHeapStart() );
}
#endif

Engine::PrimitiveTexture* Engine::D3D12GraphicInterface::GetNewPrimitiveTexture()
{
	return new D3D12PrimitiveTexture();
}

Engine::PrimitiveMesh* Engine::D3D12GraphicInterface::GetNewPrimitiveMesh()
{
	return new D3D12PrimitiveMesh();
}

Engine::GraphicPrimitiveShader* Engine::D3D12GraphicInterface::GetNewGraphicPrimitiveShader()
{
	return new D3D12GraphicPrimitiveShader();
}

Engine::ComputePrimitiveShader* Engine::D3D12GraphicInterface::GetNewComputePrimitiveShader()
{
	return new D3D12ComputePrimitiveShader();
}

Engine::PrimitiveFont* Engine::D3D12GraphicInterface::GetNewPrimitiveFont()
{
	return new D3D12PrimitiveFont();
}

Engine::PrimitiveSampler *Engine::D3D12GraphicInterface::GetNewPrimitiveSampler()
{
    return new D3D12PrimitiveSampler();
}

#if CFG_RAYTRACING
Engine::RaytracingPrimitiveShader* Engine::D3D12GraphicInterface::GetNewRaytracingShader()
{
    return new D3D12RaytracingShader();
}
#endif

Engine::GraphicInterfaceContextReturnType Engine::D3D12GraphicInterface::GetNewContext(const int8_t type, bool heap_allocation, const std::wstring_view debug_name)
{
    GraphicInterfaceContextReturnType context
    {
        m_command_pair_task_.Acquire(static_cast<D3D12_COMMAND_LIST_TYPE>(type), false, debug_name),
		heap_allocation ? m_heap_handler_->Acquire() : nullptr
    };

    return context;
}

Engine::CommandPairTask& Engine::D3D12GraphicInterface::GetCommandTask()
{
	return m_command_pair_task_;
}

void Engine::D3D12GraphicInterface::SetViewport(const GraphicInterfaceContextPrimitive* context, const Viewport& viewport)
{
    auto cmd = reinterpret_cast<CommandPair*>(context->commandList);
    const D3D12_VIEWPORT& native_viewport = reinterpret_cast<const D3D12_VIEWPORT&>(viewport);

    D3D12_RECT scissor_rect
    {
        .left = 0,
        .top = 0,
        .right = static_cast<LONG>(viewport.width),
        .bottom = static_cast<LONG>(viewport.height)
    };

    cmd->GetList()->RSSetViewports(1, &native_viewport);
    cmd->GetList()->RSSetScissorRects(1, &scissor_rect);
}

void Engine::D3D12GraphicInterface::SetDefaultRenderTarget(const GraphicInterfaceContextPrimitive* context)
{
	const auto cmd = reinterpret_cast<CommandPair*>(context->commandList);
	const auto& rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
			(
			 m_rtv_heap_->GetCPUDescriptorHandleForHeapStart(),
			 static_cast<UINT>(m_frame_idx_),
			 m_rtv_heap_size_
			);

	const auto& dsv_handle = m_dsv_heap_->GetCPUDescriptorHandleForHeapStart();

	cmd->GetList()->OMSetRenderTargets(1, &rtv_handle, false, &dsv_handle);
}

void Engine::D3D12GraphicInterface::SetDefaultGraphicPipeline(const GraphicInterfaceContextPrimitive* context)
{
	auto cmd = reinterpret_cast<CommandPair*>(context->commandList);
	cmd->GetList()->SetGraphicsRootSignature(m_pipeline_root_signature_.Get());
}

void Engine::D3D12GraphicInterface::SetDefaultComputePipeline(const GraphicInterfaceContextPrimitive* context)
{
	auto cmd = reinterpret_cast<CommandPair*>(context->commandList);
	cmd->GetList()->SetComputeRootSignature(m_pipeline_root_signature_.Get());
}

void Engine::D3D12GraphicInterface::Draw(const GraphicInterfaceContextPrimitive* context, const Resources::Mesh* mesh, const UINT instance_count, const UINT instance_offset)
{
	const auto cmd = reinterpret_cast<CommandPair*>(context->commandList);
    UINT       index_count = 0;
	if (mesh)
	{
        index_count = mesh->GetIndexCount();
        cmd->GetList()->IASetVertexBuffers(
                0, 1, static_cast<D3D12_VERTEX_BUFFER_VIEW *>( mesh->GetPrimitive()->GetNativeVertexBuffer() ) );
        cmd->GetList()->IASetIndexBuffer(
                static_cast<const D3D12_INDEX_BUFFER_VIEW *>( mesh->GetPrimitive()->GetNativeIndexBuffer() ) );
	}
	
	if ( index_count == 0 )
	{
        cmd->GetList()->DrawInstanced( 3, 1, 0, 0 );
        return;
	}

	cmd->GetList()->DrawIndexedInstanced(index_count, instance_count, 0, 0, instance_offset);
}

void Engine::D3D12GraphicInterface::Dispatch(
	const GraphicInterfaceContextPrimitive* context, const Resources::ComputeShader* shader,
	const Graphics::SBs::LocalParamSB& local_param, const UINT group_count[3]
)
{
	if (!m_local_param_)
	{
		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface(); 	
		m_local_param_ = gi.GetStructuredBuffer<Graphics::SBs::LocalParamSB>();
	}

	const auto cmd  = static_cast<CommandPair*>(context->commandList);
	const auto heap = static_cast<DescriptorPtrImpl*>(context->heap);

	SetDefaultComputePipeline(context);
	BindCompute(context, shader);
	
	m_local_param_.SetData(context, 1, &local_param);
	m_local_param_.TransitionToSRV(context);
	m_local_param_.CopySRVHeap(context);
	heap->BindCompute(context);

	cmd->GetList()->Dispatch(group_count[0], group_count[1], group_count[2]);
	m_local_param_.TransitionCommon(context);
}

void Engine::D3D12GraphicInterface::BindGraphic(const GraphicInterfaceContextPrimitive* context, const Resources::Shader* shader)
{
	const auto cmd = static_cast<const CommandPair*>(context->commandList);
	const auto heap = static_cast<DescriptorPtrImpl*>(context->heap);
	heap->SetSampler(
		static_cast<ID3D12DescriptorHeap*>(shader->GetPrimitive().GetNativeSampler())->GetCPUDescriptorHandleForHeapStart(),
		shader->GetSampler());
    cmd->GetList()->SetPipelineState( static_cast<ID3D12PipelineState *>( shader->GetPrimitive().GetNativeShader() ) );
	cmd->GetList()->IASetPrimitiveTopology(static_cast<D3D12_PRIMITIVE_TOPOLOGY>(shader->GetPrimitiveTopology()));
}

void Engine::D3D12GraphicInterface::BindCompute(
	const GraphicInterfaceContextPrimitive* context, const Resources::ComputeShader* shader
)
{
	const auto cmd = static_cast<const CommandPair*>(context->commandList);
	cmd->GetList()->SetPipelineState(static_cast<ID3D12PipelineState*>(shader->GetComputePrimitiveShader().GetNativeShader()));
}

void Engine::D3D12GraphicInterface::Transit(
	const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const D3D12_RESOURCE_STATES before,
	const D3D12_RESOURCE_STATES after
)
{
	const D3D12PrimitiveTexture* primitive = reinterpret_cast<D3D12PrimitiveTexture*>(tex->GetPrimitiveTexture());
	auto                         res       = static_cast<ID3D12Resource*>(primitive->GetNativeTexture());
	auto                         cmd       = static_cast<CommandPair*>(context->commandList);

	const auto& transition = CD3DX12_RESOURCE_BARRIER::Transition
		(
			res,
			before,
			after
		);
	
	cmd->GetList()->ResourceBarrier(1, &transition);
}

void Engine::D3D12GraphicInterface::TransitTo(
	const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const eBindType bind_type
)
{
	switch (bind_type)
	{
	case BIND_TYPE_SRV:
		{
			Transit(context, tex, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			break;
		}
	case BIND_TYPE_UAV:
		{
			Transit(context, tex, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			break;
		}
	case BIND_TYPE_RTV:
		{
			Transit(context, tex, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
			break;
		}
	case BIND_TYPE_DSV:
		{
			Transit(context, tex, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			break;
		}
	case BIND_TYPE_SAMPLER:
	case BIND_TYPE_CB:
	case BIND_TYPE_COUNT:
	default: break;
	}
}

void Engine::D3D12GraphicInterface::TransitBack(
	const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const eBindType bind_type
)
{
	switch (bind_type)
	{
	case BIND_TYPE_SRV:
		{
			Transit(context, tex, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
			break;
		}
	case BIND_TYPE_UAV:
		{
			Transit(context, tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
			break;
		}
	case BIND_TYPE_RTV:
		{
			Transit(context, tex, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
			break;
		}
	case BIND_TYPE_DSV:
		{
			Transit(context, tex, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON);
			break;
		}
	case BIND_TYPE_SAMPLER:
	case BIND_TYPE_CB:
	case BIND_TYPE_COUNT:
	default:
		break;
	}
}

void Engine::D3D12GraphicInterface::TransitMultiple(
	const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* texes, const size_t count,
	D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after
)
{
	if (count > 0)
	{
		const auto cmd  = static_cast<CommandPair*>(context->commandList);
		static std::vector<D3D12_RESOURCE_BARRIER> transitions{};
		
		if ( transitions.size() < count )
		{
            transitions.resize( count );
		}
		
		for (size_t i = 0; i < count; ++i)
		{
			const auto tex = static_cast<ID3D12Resource*>(texes[i]->GetPrimitiveTexture()->GetNativeTexture());

			const auto& transition = CD3DX12_RESOURCE_BARRIER::Transition
					(
					 tex,
					 before,
					 after
					);

			transitions[i] = transition;
		}

		cmd->GetList()->ResourceBarrier(static_cast<UINT>(count), transitions.data());
	}
}

void Engine::D3D12GraphicInterface::TransitToMultiple(
	const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* texes, const size_t count,
	const eBindType bind_type
)
{
	switch(bind_type)
	{
	case BIND_TYPE_UAV:
		TransitMultiple(context, texes, count, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		break;
	case BIND_TYPE_SRV:
		TransitMultiple(context, texes, count, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		break;
	case BIND_TYPE_DSV:
		TransitMultiple(context, texes, count, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		break;
	case BIND_TYPE_RTV:
		TransitMultiple(context, texes, count, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
		break;
	case BIND_TYPE_CB:
	case BIND_TYPE_SAMPLER:
	case BIND_TYPE_COUNT:
	default:
		break;
	}
}

void Engine::D3D12GraphicInterface::TransitBackMultiple(
	const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* texes, const size_t count,
	const eBindType bind_type
)
{
	switch(bind_type)
	{
	case BIND_TYPE_UAV:
		TransitMultiple(context, texes, count, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
		break;
	case BIND_TYPE_SRV:
		TransitMultiple(context, texes, count, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
		break;
	case BIND_TYPE_DSV:
		TransitMultiple(context, texes, count, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON);
		break;
	case BIND_TYPE_RTV:
		TransitMultiple(context, texes, count, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
		break;
	case BIND_TYPE_CB:
	case BIND_TYPE_SAMPLER:
	case BIND_TYPE_COUNT:
	default:
		break;
	}
}

void Engine::D3D12GraphicInterface::Bind(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const eBindType bind_type, const UINT slot, const UINT offset)
{
	const D3D12PrimitiveTexture* primitive = reinterpret_cast<D3D12PrimitiveTexture*>(tex->GetPrimitiveTexture());
	auto                         cmd       = static_cast<CommandPair*>(context->commandList);
	auto                         heap      = static_cast<DescriptorPtrImpl*>(context->heap);

	switch (bind_type)
	{
	case BIND_TYPE_SRV:
	{
		heap->SetShaderResource(primitive->GetSrv()->GetCPUDescriptorHandleForHeapStart(), slot + offset);
		break;
	}
	case BIND_TYPE_UAV:
	{
		heap->SetUnorderedAccess(primitive->GetUav()->GetCPUDescriptorHandleForHeapStart(), slot + offset);
		break;
	}
	case BIND_TYPE_RTV:
	{
		const D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle[]
		{
			primitive->GetRtv()->GetCPUDescriptorHandleForHeapStart()
		};

		cmd->GetList()->OMSetRenderTargets
		(
			1,
			rtv_handle,
			false,
			nullptr
		);
		break;
	}
	case BIND_TYPE_DSV:
	{
		const D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle[]
		{
			primitive->GetDsv()->GetCPUDescriptorHandleForHeapStart()
		};

		cmd->GetList()->OMSetRenderTargets
		(
			0,
			nullptr,
			false,
			dsv_handle
		);

		break;
	}
	case BIND_TYPE_SAMPLER:
	case BIND_TYPE_CB:
	case BIND_TYPE_COUNT:
	default:
		break;
	}
}

void Engine::D3D12GraphicInterface::BindMultiple(
			const GraphicInterfaceContextPrimitive* context, const Resources::Texture* const* rtvs, const size_t rtv_count,
			Resources::Texture* dsv
		)
{
	CommandPair* cmd = static_cast<CommandPair*>(context->commandList);
	
	static std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 8> rtvs_heap{};
	D3D12_CPU_DESCRIPTOR_HANDLE dsv_heap;

	if ( rtvs != nullptr )
	{
        for ( size_t i = 0; i < rtv_count; ++i )
        {
            D3D12PrimitiveTexture *dtex = static_cast<D3D12PrimitiveTexture *>( rtvs[ i ]->GetPrimitiveTexture() );
            rtvs_heap[i] = ( dtex->GetRtv()->GetCPUDescriptorHandleForHeapStart() );
        }
	}
	
	D3D12PrimitiveTexture *native_dsv = reinterpret_cast<D3D12PrimitiveTexture *>( dsv->GetPrimitiveTexture() );
    dsv_heap                          = native_dsv->GetDsv()->GetCPUDescriptorHandleForHeapStart();

	cmd->GetList()->OMSetRenderTargets
	(
		rtv_count,
		rtvs_heap.data(),
		false,
		&dsv_heap
	);
}

void Engine::D3D12GraphicInterface::BindMultiple(
	const GraphicInterfaceContextPrimitive* context,
	const Resources::Texture* const* textures,
	const eBindType bind_type,
	const UINT slot,
	const UINT offset,
	const size_t count)
{
	const auto heap = static_cast<DescriptorPtrImpl*>(context->heap);
	heap->SetShaderResources(textures, count, slot + offset);
}

void Engine::D3D12GraphicInterface::Clear(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex, const eBindType clear_type)
{
	const auto primitive   = static_cast<D3D12PrimitiveTexture*>(tex->GetPrimitiveTexture());
	const auto resource = static_cast<ID3D12Resource*>(primitive->GetNativeTexture());
	const auto cmd = reinterpret_cast<CommandPair*>(context->commandList);
	
	if (clear_type == BIND_TYPE_RTV)
	{
		constexpr float clear_color[4] = {0.f, 0.f, 0.f, 1.f};
		const auto& transition = CD3DX12_RESOURCE_BARRIER::Transition
				(
				 resource,
				 D3D12_RESOURCE_STATE_COMMON,
				 D3D12_RESOURCE_STATE_RENDER_TARGET
				);

		const auto& transition_back = CD3DX12_RESOURCE_BARRIER::Transition
				(
				 resource,
				 D3D12_RESOURCE_STATE_RENDER_TARGET,
				 D3D12_RESOURCE_STATE_COMMON
				);

		cmd->GetList()->ResourceBarrier(1, &transition);

		cmd->GetList()->ClearRenderTargetView
				(
				 primitive->GetRtv()->GetCPUDescriptorHandleForHeapStart(),
				 clear_color,
				 0,
				 nullptr
				);

		cmd->GetList()->ResourceBarrier(1, &transition_back);
	}
	else if (clear_type == BIND_TYPE_DSV)
	{
		const auto& transition = CD3DX12_RESOURCE_BARRIER::Transition
				(
				 resource,
				 D3D12_RESOURCE_STATE_COMMON,
				 D3D12_RESOURCE_STATE_DEPTH_WRITE
				);

		const auto& transition_back = CD3DX12_RESOURCE_BARRIER::Transition
				(
				 resource,
				 D3D12_RESOURCE_STATE_DEPTH_WRITE,
				 D3D12_RESOURCE_STATE_COMMON
				);

		cmd->GetList()->ResourceBarrier(1, &transition);

		cmd->GetList()->ClearDepthStencilView
				(
				 primitive->GetDsv()->GetCPUDescriptorHandleForHeapStart(),
				 D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
				 1.f,
				 0,
				 0,
				 nullptr
				);

		cmd->GetList()->ResourceBarrier(1, &transition_back);
	}
}

void Engine::D3D12GraphicInterface::ClearRenderTarget()
{
	const Strong<CommandPair>& cmd = m_command_pair_task_.Acquire(D3D12_COMMAND_LIST_TYPE_DIRECT, false, L"Render Target Clear").lock();
	cmd->SoftReset();

	constexpr float color[4] = { 0.f, 0.f, 0.f, 1.f };
	const auto& rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
	(
		m_rtv_heap_->GetCPUDescriptorHandleForHeapStart(),
		static_cast<UINT>(m_frame_idx_),
		m_rtv_heap_size_
	);

	const auto& dsv_handle = m_dsv_heap_->GetCPUDescriptorHandleForHeapStart();
	const auto initial_barrier = CD3DX12_RESOURCE_BARRIER::Transition
			(
			 m_render_targets_[m_frame_idx_].Get(),
			 D3D12_RESOURCE_STATE_PRESENT,
			 D3D12_RESOURCE_STATE_RENDER_TARGET
			);

	cmd->GetList()->ResourceBarrier(1, &initial_barrier);

	cmd->GetList()->ClearRenderTargetView(rtv_handle, color, 0, nullptr);
	cmd->GetList()->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	cmd->FlagReady();
}

void Engine::D3D12GraphicInterface::CopyRenderTarget(const GraphicInterfaceContextPrimitive* context, const Resources::Texture* tex)
{
	auto cmd = reinterpret_cast<CommandPair*>(context->commandList);
	auto* resource = static_cast<ID3D12Resource*>(tex->GetPrimitiveTexture()->GetNativeTexture());

	const auto& dst_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		resource,
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST
	);

	const auto& dst_transition_back = CD3DX12_RESOURCE_BARRIER::Transition
	(
		resource,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON
	);

	const auto& copy_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_render_targets_[m_frame_idx_].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_COPY_SOURCE
	);

	const auto& rtv_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_render_targets_[m_frame_idx_].Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	cmd->GetList()->ResourceBarrier(1, &copy_transition);
	cmd->GetList()->ResourceBarrier(1, &dst_transition);
	cmd->GetList()->CopyResource(resource, m_render_targets_[m_frame_idx_].Get());
	cmd->GetList()->ResourceBarrier(1, &rtv_transition);
	cmd->GetList()->ResourceBarrier(1, &dst_transition_back);
}

Engine::StructuredBufferTypeless* Engine::D3D12GraphicInterface::GetNativeStructuredBuffer()
{
	return new Graphics::D3D12StructuredBufferTypeless();
}

Engine::ConstantBufferTypeless* Engine::D3D12GraphicInterface::GetNativeConstantBuffer()
{
	return new Graphics::D3D12ConstantBufferTypeless();
}

void Engine::D3D12GraphicInterface::InitializeDevice()
{
#if WITH_DEBUG
	ComPtr<ID3D12Debug>  debug_interface;
	ComPtr<ID3D12Debug1> debug_interface1;
	DX::ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
	DX::ThrowIfFailed(debug_interface->QueryInterface(IID_PPV_ARGS(debug_interface1.GetAddressOf())));
	debug_interface->EnableDebugLayer();
#endif

#if WITH_DEBUG & defined(SNIFF_DEVICE_REMOVAL)
	ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dred_settings;
	DX::ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&dred_settings)));

	// Turn on auto-breadcrumbs and page fault reporting.
	dred_settings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	dred_settings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
#endif
	// Create factory and Searching for adapter
	ComPtr<IDXGIFactory4> dxgi_factory;

	UINT dxgi_factory_flags = 0;

#if WITH_DEBUG
	dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	DX::ThrowIfFailed(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));

	ComPtr<IDXGIAdapter1> adapter;
	int                   adapter_idx = 0;

	while (dxgi_factory->EnumAdapters1(adapter_idx, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapter_idx++;
			continue;
		}

		if (SUCCEEDED
		(
			D3D12CreateDevice
			(
				adapter.Get(), D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(m_dev_.GetAddressOf()) // this macro replaced with uuidof and address.
			)
		))
		{
			break;
		}

		adapter_idx++;
	}

	if (!adapter)
	{
		throw std::runtime_error("Failed to find a suitable adapter.");
	}

	InitializePipeline();

	m_command_pair_task_.Initialize(m_dev_.Get(), m_heap_handler_, CFG_FRAME_BUFFER);

	// Create swap chain
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};

	swap_chain_desc.BufferCount = 2;
	swap_chain_desc.Width = CFG_WIDTH;
	swap_chain_desc.Height = CFG_HEIGHT;
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swap_chain_desc.Scaling = DXGI_SCALING_NONE;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC full_screen_desc = {};

	full_screen_desc.Windowed = !CFG_FULLSCREEN;
	full_screen_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	if constexpr (CFG_VSYNC)
	{
		full_screen_desc.RefreshRate.Denominator = s_refresh_rate_denominator_;
		full_screen_desc.RefreshRate.Numerator = s_refresh_rate_numerator_;
	}
	else
	{
		full_screen_desc.RefreshRate.Denominator = 1;
		full_screen_desc.RefreshRate.Numerator = 0;
	}

	DX::ThrowIfFailed
	(
		dxgi_factory->CreateSwapChainForHwnd
		(
			m_command_pair_task_.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
			WinAPI::WinAPIWrapper::GetHWND(),
			&swap_chain_desc,
			&full_screen_desc,
			nullptr,
			(IDXGISwapChain1**)m_swap_chain_.GetAddressOf()
		)
	);

	DX::ThrowIfFailed(m_swap_chain_->SetMaximumFrameLatency(CFG_FRAME_LATENCY_TOLERANCE_SECOND));
	m_frame_idx_ = m_swap_chain_->GetCurrentBackBufferIndex();

	m_render_targets_.resize(CFG_FRAME_BUFFER);

	for (UINT i = 0; i < CFG_FRAME_BUFFER; ++i)
	{
		DX::ThrowIfFailed
		(
			m_swap_chain_->GetBuffer
			(
				i,
				IID_PPV_ARGS(m_render_targets_[i].GetAddressOf())
			)
		);
	}

	const D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc
	{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = CFG_FRAME_BUFFER,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
	};

	DX::ThrowIfFailed
	(
		m_dev_->CreateDescriptorHeap
		(
			&rtv_heap_desc,
			IID_PPV_ARGS(m_rtv_heap_.GetAddressOf())
		)
	);

	m_rtv_heap_size_ = m_dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_heap_->GetCPUDescriptorHandleForHeapStart());

	constexpr D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
	{
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
		.Texture2D = {0, 0}
	};

	for (UINT i = 0; i < CFG_FRAME_BUFFER; ++i)
	{
		DX::ThrowIfFailed
		(
			m_swap_chain_->GetBuffer
			(
				i, IID_PPV_ARGS(m_render_targets_[i].ReleaseAndGetAddressOf())
			)
		);

		const std::wstring name = L"Render Target " + std::to_wstring(i);

		DX::ThrowIfFailed(m_render_targets_[i]->SetName(name.c_str()));

		m_dev_->CreateRenderTargetView
		(
			m_render_targets_[i].Get(), &rtv_desc, rtv_handle
		);

		rtv_handle.Offset(1, m_rtv_heap_size_);
	}

	const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	const CD3DX12_RESOURCE_DESC& depth_desc = CD3DX12_RESOURCE_DESC::Tex2D
	(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		CFG_WIDTH,
		CFG_HEIGHT,
		1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	constexpr D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc
	{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
	};

	DX::ThrowIfFailed
	(
		m_dev_->CreateDescriptorHeap
		(
			&descriptor_heap_desc, IID_PPV_ARGS(m_dsv_heap_.ReleaseAndGetAddressOf())
		)
	);

	constexpr D3D12_CLEAR_VALUE clear_value
	{
		.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
		.DepthStencil = {1.0f, 0}
	};

	DX::ThrowIfFailed
	(
		m_dev_->CreateCommittedResource
		(
			&default_heap,
			D3D12_HEAP_FLAG_NONE,
			&depth_desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clear_value,
			IID_PPV_ARGS(m_depth_stencil_.ReleaseAndGetAddressOf())
		)
	);

	m_dev_->CreateDepthStencilView
	(
		m_depth_stencil_.Get(), nullptr, m_dsv_heap_->GetCPUDescriptorHandleForHeapStart()
	);

#if WITH_DEBUG
	ComPtr<ID3D12InfoQueue> info_queue;
	if (SUCCEEDED(m_dev_.As(&info_queue)))
	{
		DX::ThrowIfFailed(info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true));
		DX::ThrowIfFailed(info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true));

		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_MESSAGE_ID deny_ids[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
		};

		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		filter.DenyList.NumIDs = _countof(deny_ids);
		filter.DenyList.pIDList = deny_ids;

		DX::ThrowIfFailed(info_queue->PushStorageFilter(&filter));
	}
#endif
}

void Engine::D3D12GraphicInterface::InitializePipeline()
{
	CD3DX12_DESCRIPTOR_RANGE1 ranges[RASTERIZER_SLOT_COUNT];
	ranges[RASTERIZER_SLOT_SRV].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, g_max_engine_texture_slots, 0);
	ranges[RASTERIZER_SLOT_CB].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, g_max_cb_slots, 0);
	ranges[RASTERIZER_SLOT_UAV].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, g_max_uav_slots, 0);
	ranges[RASTERIZER_SLOT_SAMPLER].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, SAMPLER_END, 0);

	ranges[RASTERIZER_SLOT_SRV].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	ranges[RASTERIZER_SLOT_CB].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	ranges[RASTERIZER_SLOT_UAV].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	ranges[RASTERIZER_SLOT_SAMPLER].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;


	CD3DX12_ROOT_PARAMETER1 root_parameters[RASTERIZER_SLOT_COUNT];
	root_parameters[RASTERIZER_SLOT_SRV].InitAsDescriptorTable
	(1, &ranges[RASTERIZER_SLOT_SRV], D3D12_SHADER_VISIBILITY_ALL);
	root_parameters[RASTERIZER_SLOT_CB].InitAsDescriptorTable
	(1, &ranges[RASTERIZER_SLOT_CB], D3D12_SHADER_VISIBILITY_ALL);
	root_parameters[RASTERIZER_SLOT_UAV].InitAsDescriptorTable
	(1, &ranges[RASTERIZER_SLOT_UAV], D3D12_SHADER_VISIBILITY_ALL);
	root_parameters[RASTERIZER_SLOT_SAMPLER].InitAsDescriptorTable
	(1, &ranges[RASTERIZER_SLOT_SAMPLER], D3D12_SHADER_VISIBILITY_ALL);

	const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc
	(
		RASTERIZER_SLOT_COUNT,
		root_parameters,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ComPtr<ID3DBlob> signature_blob = nullptr;
	ComPtr<ID3DBlob> error_blob = nullptr;

	DX::ThrowIfFailed
	(
		D3D12SerializeVersionedRootSignature
		(
			&root_signature_desc,
			signature_blob.GetAddressOf(),
			error_blob.GetAddressOf()
		));

	if (error_blob)
	{
		const std::string error_message =
			static_cast<char*>(error_blob->GetBufferPointer());

		OutputDebugStringA(error_message.c_str());
	}

	DX::ThrowIfFailed
	(
		m_dev_->CreateRootSignature
		(
			0,
			signature_blob->GetBufferPointer(),
			signature_blob->GetBufferSize(),
			IID_PPV_ARGS(m_pipeline_root_signature_.ReleaseAndGetAddressOf())
		)
	);

	m_heap_handler_ = boost::make_shared<decltype(m_heap_handler_)::element_type>();
	m_heap_handler_->Initialize(m_dev_.Get(), m_pipeline_root_signature_.Get());
}

void Engine::D3D12GraphicInterface::DetachCommandThread()
{
    m_command_task_thread_ = std::thread(&CommandPairTask::StartTask, &m_command_pair_task_);
    m_command_task_thread_.detach();
}

float Engine::D3D12GraphicInterface::GetAspectRatio()
{
	return CFG_WIDTH / CFG_HEIGHT;
}

Matrix Engine::D3D12GraphicInterface::GetProjectionMatrix()
{
	return m_projection_matrix_;
}

Matrix Engine::D3D12GraphicInterface::GetOrthogonalMatrix()
{
	return m_ortho_matrix_;
}

Engine::Strong<Engine::CommandListBase> Engine::D3D12GraphicInterface::GetCommandList(const int8_t type, const std::wstring_view debug_name)
{
	return m_command_pair_task_.Acquire(static_cast<D3D12_COMMAND_LIST_TYPE>(type), false, debug_name).lock();
}

Engine::Unique<Engine::GraphicHeapBase> Engine::D3D12GraphicInterface::GetHeap()
{
	return std::move(m_heap_handler_->Acquire());
}

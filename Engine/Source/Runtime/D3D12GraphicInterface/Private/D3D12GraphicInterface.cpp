#include "D3D12GraphicInterface.h"
#include "ThrowIfFailed.h"

#include "StructuredBufferDX12.hpp"
#include "Source/Runtime/Managers/WinAPIWrapper/Public/WinAPIWrapper.hpp"
#include "Source/Runtime/Resources/Texture/Public/Texture.h"

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

	m_command_pair_task_.SwapBuffer(m_swap_chain_->GetCurrentBackBufferIndex());
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
	cmd->FlagReady();

	m_command_pair_task_.WaitForCommandsCompletion();

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

Engine::GraphicInterfaceContextReturnType Engine::D3D12GraphicInterface::GetNewContext(const int8_t type, bool heap_allocation, const std::wstring_view debug_name)
{
    GraphicInterfaceContextReturnType context
    {
        m_command_pair_task_.Acquire(static_cast<D3D12_COMMAND_LIST_TYPE>(type), false, debug_name),
		heap_allocation ? m_heap_handler_->Acquire() : nullptr
    };

    return context;
}

void Engine::D3D12GraphicInterface::SetViewport(const GraphicInterfaceContextPrimitive* context, const Viewport& viewport)
{
    auto cmd = reinterpret_cast<CommandPair*>(context->commandList);
    const D3D12_VIEWPORT& native_viewport = reinterpret_cast<const D3D12_VIEWPORT&>(viewport);

    D3D12_RECT scissor_rect
    {
        .left = 0,
        .top = 0,
        .right = viewport.width,
        .bottom = viewport.height
    };

    cmd->GetList()->RSSetViewports(1, &native_viewport);
    cmd->GetList()->RSSetScissorRects(1, &scissor_rect);
}

void Engine::D3D12GraphicInterface::SetDefaultPipeline(const GraphicInterfaceContextPrimitive* context)
{
	auto cmd = reinterpret_cast<CommandPair*>(context->commandList);
	cmd->GetList()->SetGraphicsRootSignature(m_pipeline_root_signature_.Get());
}

void Engine::D3D12GraphicInterface::Draw(const GraphicInterfaceContextPrimitive* context, Resources::Shape* shape, const UINT instance_count)
{
}

void Engine::D3D12GraphicInterface::Draw(const GraphicInterfaceContextPrimitive* context, Resources::Mesh* mesh, const UINT instance_count)
{
}

void Engine::D3D12GraphicInterface::Bind(const GraphicInterfaceContextPrimitive* context, Resources::Shader* shader)
{
}

void Engine::D3D12GraphicInterface::Bind(const GraphicInterfaceContextPrimitive* context, Resources::Texture* tex, const eBindType bind_type, const UINT slot, const UINT offset)
{
}

void Engine::D3D12GraphicInterface::Unbind(const GraphicInterfaceContextPrimitive* context, Resources::Texture* tex, const eBindType bind_type)
{
}

void Engine::D3D12GraphicInterface::Clear(const GraphicInterfaceContextPrimitive* context, Resources::Texture* tex, const eBindType clear_type)
{
}

void Engine::D3D12GraphicInterface::ClearRenderTarget()
{
	Strong<CommandPair> cmd = m_command_pair_task_.Acquire(D3D12_COMMAND_LIST_TYPE_DIRECT, false, L"Render Target Clear").lock();
	cmd->SoftReset();

	constexpr float color[4] = { 0.f, 0.f, 0.f, 1.f };
	const auto& rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
	(
		m_rtv_heap_->GetCPUDescriptorHandleForHeapStart(),
		static_cast<UINT>(m_frame_idx_),
		m_rtv_heap_size_
	);

	const auto& dsv_handle = m_dsv_heap_->GetCPUDescriptorHandleForHeapStart();

	cmd->GetList()->ClearRenderTargetView(rtv_handle, color, 0, nullptr);
	cmd->GetList()->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	cmd->FlagReady();
}

void Engine::D3D12GraphicInterface::CopyRenderTarget(const GraphicInterfaceContextPrimitive* context, Resources::Texture* tex)
{
	auto cmd = reinterpret_cast<CommandPair*>(context->commandList);
	auto* resource = reinterpret_cast<ID3D12Resource*>(tex->GetPrimitiveTexture()->GetPrimitiveTexture());

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

Engine::Unique<Engine::StructuredBufferTypelessBase>&& Engine::D3D12GraphicInterface::GetNativeStructuredBuffer()
{
	return std::move(std::make_unique<Engine::Graphics::DXStructuredBufferTypeless>());
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
	if (SUCCEEDED(m_device_.As(&info_queue)))
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

	m_heap_handler_ = std::make_shared<DescriptorHandler>();
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

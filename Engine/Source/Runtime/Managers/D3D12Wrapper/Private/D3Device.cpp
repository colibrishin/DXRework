#include "../Public/D3Device.hpp"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxcompiler.lib")

#include <dxcapi.h>
#include <dxgi1_5.h>
#include <d3dcompiler.h>
#include <directxtk12/DDSTextureLoader.h>
#include <directxtk12/WICTextureLoader.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <directx/d3dx12_core.h>
#include <boost/filesystem.hpp>

#include "Source/Runtime/ThrowIfFailed/Public/ThrowIfFailed.h"
#include <Source/Runtime/CommandPair/Public/CommandPair.h>
#include "Source/Runtime/Managers/WinAPIWrapper/Public/WinAPIWrapper.hpp"

std::atomic<bool> g_raytracing = false;

namespace Engine::Managers
{
	HANDLE D3Device::GetSwapchainAwaiter() const
	{
		return m_swap_chain_->GetFrameLatencyWaitableObject();
	}

	ID3D12GraphicsCommandList1* D3Device::GetCommandList(const eCommandList list_enum, UINT64 frame_idx) const
	{
		if (frame_idx == -1)
		{
			frame_idx = m_frame_idx_;
		}

		return m_command_pairs_.at(frame_idx)[list_enum]->GetList();
	}

	ID3D12CommandQueue* D3Device::GetCommandQueue(const eCommandList list) const
	{
		return m_command_queues_[s_target_types[list]].Get();
	}

	ID3D12CommandQueue* D3Device::GetCommandQueue(const eCommandTypes type) const
	{
		return m_command_queues_[type].Get();
	}

	std::vector<std::pair<D3D12_INPUT_ELEMENT_DESC, std::string>> D3Device::GenerateInputDescription(
		ID3DBlob* blob
	)
	{
		ComPtr<ID3D12ShaderReflection> reflection = nullptr;

		DX::ThrowIfFailed
				(
				 D3DReflect
				 (
				  blob->GetBufferPointer(), blob->GetBufferSize(),
				  IID_ID3D11ShaderReflection,
				  reinterpret_cast<void**>(reflection.GetAddressOf())
				 )
				);

		std::vector<std::pair<D3D12_INPUT_ELEMENT_DESC, std::string>> input_descs_with_name;

		D3D12_SHADER_DESC desc{};
		DX::ThrowIfFailed(reflection->GetDesc(&desc));

		UINT byteOffset = 0;

		for (UINT i = 0; i < desc.InputParameters; ++i)
		{
			D3D12_SIGNATURE_PARAMETER_DESC param_desc;
			D3D12_INPUT_ELEMENT_DESC       input_desc{};
			DX::ThrowIfFailed(reflection->GetInputParameterDesc(i, &param_desc));

			std::string name_buffer(param_desc.SemanticName);

			input_desc.SemanticName         = name_buffer.c_str();
			input_desc.SemanticIndex        = param_desc.SemanticIndex;
			input_desc.InputSlot            = 0;
			input_desc.AlignedByteOffset    = byteOffset;
			input_desc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			input_desc.InstanceDataStepRate = 0;

			// determine DXGI format
			if (param_desc.Mask == 1)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32_UINT;
				}
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32_SINT;
				}
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				{
					input_desc.Format = DXGI_FORMAT_R32_FLOAT;
				}
				byteOffset += 4;
			}
			else if (param_desc.Mask <= 3)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32_UINT;
				}
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32_SINT;
				}
				else if (param_desc.ComponentType ==
				         D3D_REGISTER_COMPONENT_FLOAT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
				}
				byteOffset += 8;
			}
			else if (param_desc.Mask <= 7)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32B32_UINT;
				}
				else if (param_desc.ComponentType ==
				         D3D_REGISTER_COMPONENT_SINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32B32_SINT;
				}
				else if (param_desc.ComponentType ==
				         D3D_REGISTER_COMPONENT_FLOAT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
				}
				byteOffset += 12;
			}
			else if (param_desc.Mask <= 15)
			{
				if (param_desc.ComponentType ==
				    D3D_REGISTER_COMPONENT_UINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
				}
				else if (param_desc.ComponentType ==
				         D3D_REGISTER_COMPONENT_SINT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
				}
				else if (param_desc.ComponentType ==
				         D3D_REGISTER_COMPONENT_FLOAT32)
				{
					input_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				}
				byteOffset += 16;
			}

			input_descs_with_name.emplace_back(input_desc, name_buffer);
		}

		for (auto& dsc : input_descs_with_name)
		{
			dsc.first.SemanticName = dsc.second.c_str();
		}

		return input_descs_with_name;
	}

	D3Device::~D3Device()
	{
		WaitForCommandsCompletion();
		m_command_consumer_running_ = false;
		CloseHandle(m_fence_event_);
		delete[] m_fence_nonce_;
	}

	void D3Device::InitializeDevice()
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
					 IID_PPV_ARGS(m_device_.GetAddressOf()) // this macro replaced with uuidof and address.
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

		// If an acceptable adapter is found, create a device and a command queue.
		constexpr D3D12_COMMAND_QUEUE_DESC queue_descs[]
		{
			{
				.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
				.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
				.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 0
			},
			{
				.Type = D3D12_COMMAND_LIST_TYPE_COPY,
				.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
				.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 0
			},
			{
				.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
				.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
				.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 0
			}
		};

		for (int i = 0; i < COMMAND_TYPE_COUNT; ++i)
		{
			DX::ThrowIfFailed
					(
					 m_device_->CreateCommandQueue
					 (
					  &queue_descs[i],
					  IID_PPV_ARGS(m_command_queues_[i].GetAddressOf())
					 )
					);
		}

		// Create swap chain
		DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};

		swap_chain_desc.BufferCount        = 2;
		swap_chain_desc.Width              = CFG_WIDTH;
		swap_chain_desc.Height             = CFG_HEIGHT;
		swap_chain_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.SampleDesc.Count   = 1;
		swap_chain_desc.SampleDesc.Quality = 0;
		swap_chain_desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
		swap_chain_desc.Scaling            = DXGI_SCALING_NONE;
		swap_chain_desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swap_chain_desc.Flags              = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC full_screen_desc = {};

		full_screen_desc.Windowed         = !CFG_FULLSCREEN;
		full_screen_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

		if (CFG_VSYNC)
		{
			full_screen_desc.RefreshRate.Denominator = s_refresh_rate_denominator_;
			full_screen_desc.RefreshRate.Numerator   = s_refresh_rate_numerator_;
		}
		else
		{
			full_screen_desc.RefreshRate.Denominator = 1;
			full_screen_desc.RefreshRate.Numerator   = 0;
		}

		DX::ThrowIfFailed
				(
				 dxgi_factory->CreateSwapChainForHwnd
				 (
				  m_command_queues_[COMMAND_TYPE_DIRECT].Get(),
				  m_hwnd_,
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
				 m_device_->CreateDescriptorHeap
				 (
				  &rtv_heap_desc,
				  IID_PPV_ARGS(m_rtv_heap_.GetAddressOf())
				 )
				);

		m_rtv_heap_size_ = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

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

			GetDevice()->CreateRenderTargetView
					(
					 m_render_targets_[i].Get(), &rtv_desc, rtv_handle
					);

			rtv_handle.Offset(1, m_rtv_heap_size_);
		}

		const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		const CD3DX12_RESOURCE_DESC& depth_desc   = CD3DX12_RESOURCE_DESC::Tex2D
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
				 GetDevice()->CreateDescriptorHeap
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
				 GetDevice()->CreateCommittedResource
				 (
				  &default_heap,
				  D3D12_HEAP_FLAG_NONE,
				  &depth_desc,
				  D3D12_RESOURCE_STATE_DEPTH_WRITE,
				  &clear_value,
				  IID_PPV_ARGS(m_depth_stencil_.ReleaseAndGetAddressOf())
				 )
				);

		GetDevice()->CreateDepthStencilView
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
			filter.DenyList.NumSeverities  = _countof(severities);
			filter.DenyList.pSeverityList  = severities;
			filter.DenyList.NumIDs         = _countof(deny_ids);
			filter.DenyList.pIDList        = deny_ids;

			DX::ThrowIfFailed(info_queue->PushStorageFilter(&filter));
		}
#endif
	}

	void D3Device::InitializeCommandAllocator()
	{
		for (UINT i = 0; i < CFG_FRAME_BUFFER; ++i)
		{
			for (int t = 0; t < _countof(s_target_types); ++t)
			{
				m_command_pairs_[i].emplace_back
						(boost::make_shared<CommandPair>(s_target_types[t], ++m_command_ids_, m_frame_idx_, L""));
			}
		}
	}

	void D3Device::InitializeFence()
	{
		DX::ThrowIfFailed
				(
				 m_device_->CreateFence
				 (
				  0, D3D12_FENCE_FLAG_NONE,
				  IID_PPV_ARGS(m_fence_.GetAddressOf())
				 )
				);

		m_fence_nonce_ = new std::atomic<UINT64>[CFG_FRAME_BUFFER]{0,};

		m_fence_event_ = CreateEvent(nullptr, false, false, nullptr);

		if (m_fence_event_ == nullptr)
		{
			DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
	}

	void D3Device::InitializeConsumer()
	{
		m_command_consumer_running_ = true;
		m_command_consumer_         = std::thread(&D3Device::ConsumeCommands, this);
		m_command_consumer_.detach();
	}

	void D3Device::WaitForEventCompletion(const UINT64 buffer_idx) const
	{
		if (m_fence_->GetCompletedValue() < m_fence_nonce_[buffer_idx])
		{
			DX::ThrowIfFailed
					(
					 m_fence_->SetEventOnCompletion
					 (
					  m_fence_nonce_[buffer_idx],
					  m_fence_event_
					 )
					);

			WaitForSingleObject(m_fence_event_, INFINITE);
		}
	}

	void D3Device::WaitForCommandsCompletion() const
	{
		while (true)
		{
			const UINT64 count = m_command_pairs_count_.load();

			if (count == 0)
			{
				break;
			}

			m_command_pairs_count_.wait(count);
		}
	}

	void D3Device::PreUpdate(const float& dt) {}

	void D3Device::Update(const float& dt) {}

	void D3Device::PreRender(const float& dt) {}

	void D3Device::Render(const float& dt) {}

	void D3Device::FixedUpdate(const float& dt) {}

	void D3Device::PostRender(const float& dt)
	{
		const auto& cmd = AcquireCommandPair(L"Finalize Render").lock();

		const auto present_barrier = CD3DX12_RESOURCE_BARRIER::Transition
				(
				 GetRenderTarget(m_frame_idx_),
				 D3D12_RESOURCE_STATE_RENDER_TARGET,
				 D3D12_RESOURCE_STATE_PRESENT
				);

		cmd->SoftReset();
		cmd->GetList()->ResourceBarrier(1, &present_barrier);
		cmd->FlagReady();

		WaitForCommandsCompletion();

		DXGI_PRESENT_PARAMETERS params;
		params.DirtyRectsCount = 0;
		params.pDirtyRects     = nullptr;
		params.pScrollRect     = nullptr;
		params.pScrollOffset   = nullptr;

		DX::ThrowIfFailed
				(
				 m_swap_chain_->Present1
				 (
				  CFG_VSYNC ? 1 : 0, DXGI_PRESENT_DO_NOT_WAIT,
				  &params
				 )
				);

		if (WaitForSingleObjectEx
		    (
		     GetSwapchainAwaiter(), CFG_FRAME_LATENCY_TOLERANCE_SECOND * 1000,
		     true
		    ) != WAIT_OBJECT_0)
		{
#if WITH_DEBUG
			OutputDebugString("Waiting for Swap chain had an issue.");
#endif
		}

		WaitNextFrame();
	}

	void D3Device::PostUpdate(const float& dt) {}

	void D3Device::Initialize()
	{
		m_hwnd_ = WinAPI::WinAPIWrapper::GetHWND();

		InitializeDevice();
		InitializeCommandAllocator();
		InitializeFence();
		InitializeConsumer();

		m_projection_matrix_ = XMMatrixPerspectiveFovLH
				(
				 CFG_FOV, GetAspectRatio(),
				 CFG_SCREEN_NEAR, CFG_SCREEN_FAR
				);
		m_ortho_matrix_ = XMMatrixOrthographicLH
				(
				 static_cast<float>(CFG_WIDTH),
				 static_cast<float>(CFG_HEIGHT),
				CFG_SCREEN_NEAR, CFG_SCREEN_FAR
			);
	}

	ID3D12Resource* D3Device::GetRenderTarget(UINT64 frame_idx)
	{
		return m_render_targets_[frame_idx].Get();
	}

	void D3Device::CreateTextureFromFile(
		const boost::filesystem::path& file_path,
		ID3D12Resource**             res,
		bool                         generate_mip = false
	) const
	{
		if (!boost::filesystem::exists(file_path))
		{
			throw std::runtime_error("File not found.");
		}

		DirectX::ResourceUploadBatch resource_upload_batch(m_device_.Get());

		resource_upload_batch.Begin();

		if (file_path.extension() == ".dds")
		{
			DX::ThrowIfFailed
					(
					 DirectX::CreateDDSTextureFromFile
					 (
					  m_device_.Get(),
					  resource_upload_batch,
					  file_path.c_str(),
					  res,
					  generate_mip
					 )
					);
		}
		else
		{
			DX::ThrowIfFailed
					(
					DirectX::CreateWICTextureFromFile
					 (
					  m_device_.Get(),
					  resource_upload_batch,
					  file_path.c_str(),
					  res,
					  generate_mip
					 )
					);
		}

		const auto& token = resource_upload_batch.End(GetCommandQueue(COMMAND_LIST_UPDATE));
		token.wait();
	}

	void D3Device::DefaultRenderTarget(const Weak<CommandPair>& w_cmd) const
	{
		if (const auto& cmd = w_cmd.lock())
		{
			const auto& rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
					(
					 m_rtv_heap_->GetCPUDescriptorHandleForHeapStart(),
					 static_cast<UINT>(m_frame_idx_),
					 m_rtv_heap_size_
					);

			const auto& dsv_handle = m_dsv_heap_->GetCPUDescriptorHandleForHeapStart();

			cmd->GetList()->OMSetRenderTargets(1, &rtv_handle, false, &dsv_handle);
		}
	}

	void D3Device::CopyBackBuffer(const Weak<CommandPair>& w_cmd, ID3D12Resource* resource) const
	{
		if (const auto& cmd = w_cmd.lock())
		{
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
	}

	void D3Device::ClearRenderTarget(bool barrier)
	{
		const auto& cmd = AcquireCommandPair(L"Cleanup").lock();

		cmd->SoftReset();

		constexpr float color[4]   = {0.f, 0.f, 0.f, 1.f};
		const auto&     rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
				(
				 GetRTVHeap()->GetCPUDescriptorHandleForHeapStart(),
				 static_cast<UINT>(GetFrameIndex()),
				 GetRTVHeapSize()
				);

		const auto& dsv_handle = GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

		if (barrier)
		{
			const auto initial_barrier = CD3DX12_RESOURCE_BARRIER::Transition
					(
					 GetRenderTarget(GetFrameIndex()),
					 D3D12_RESOURCE_STATE_PRESENT,
					 D3D12_RESOURCE_STATE_RENDER_TARGET
					);

			cmd->GetList()->ResourceBarrier(1, &initial_barrier);
		}

		cmd->GetList()->ClearRenderTargetView(rtv_handle, color, 0, nullptr);
		cmd->GetList()->ClearDepthStencilView
				(dsv_handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		cmd->FlagReady();
	}

	ID3D12DescriptorHeap* D3Device::GetRTVHeap() const
	{
		return m_rtv_heap_.Get();
	}

	ID3D12DescriptorHeap* D3Device::GetDSVHeap() const
	{
		return m_dsv_heap_.Get();
	}

	UINT D3Device::GetRTVHeapSize() const
	{
		return m_rtv_heap_size_;
	}

	float D3Device::GetAspectRatio()
	{
		return static_cast<float>(CFG_WIDTH) /
		       static_cast<float>(CFG_HEIGHT);
	}

	Weak<CommandPair> D3Device::AcquireCommandPair(const std::wstring& debug_name, UINT64 buffer_idx)
	{
		if (buffer_idx == -1)
		{
			buffer_idx = m_frame_idx_;
		}

		std::lock_guard<std::mutex> lock(m_command_pairs_mutex_);
		const UINT64                next = ++m_command_ids_;

		m_command_pairs_generated_.push(s_command_pair_pool.allocate(COMMAND_TYPE_DIRECT, next, buffer_idx, debug_name));
		m_command_pairs_count_.fetch_add(1);

		return m_command_pairs_generated_.back();
	}

	bool D3Device::IsCommandPairAvailable(UINT64 buffer_idx) const
	{
		return m_command_pairs_count_.load() < CFG_MAX_CONCURRENT_COMMAND_LIST;
	}

	void D3Device::WaitAndReset(const eCommandList list, UINT64 buffer_idx) const
	{
		if (buffer_idx == -1)
		{
			buffer_idx = m_frame_idx_;
		}

		WaitForEventCompletion(buffer_idx);

		const auto& list_pair = m_command_pairs_.at(buffer_idx)[list];

		list_pair->SoftReset();
	}

	void D3Device::Wait(UINT64 buffer_idx) const
	{
		if (buffer_idx == -1)
		{
			buffer_idx = m_frame_idx_;
		}

		WaitForEventCompletion(buffer_idx);
	}

	void D3Device::WaitNextFrame()
	{
		const auto& next_idx = m_swap_chain_->GetCurrentBackBufferIndex();

		m_fence_nonce_[next_idx] = GetFenceValue(m_frame_idx_);

		Signal(COMMAND_TYPE_DIRECT, next_idx);

		WaitForEventCompletion(next_idx);

		m_frame_idx_ = next_idx;

		for (int i = 0; i < COMMAND_LIST_COUNT; ++i)
		{
			m_command_pairs_[m_frame_idx_][i]->HardReset();
		}
	}

	void D3Device::ConsumeCommands()
	{
		while (m_command_consumer_running_)
		{
			if (const UINT64 count = m_command_pairs_count_.load())
			{
				std::lock_guard<std::mutex> lock(m_command_pairs_mutex_);

				if (m_command_pairs_generated_.empty())
				{
					continue;
				}

				if (const auto& queued = m_command_pairs_generated_.front().lock();
					queued && (queued->IsExecuted() || queued->IsDisposed()))
				{
					m_command_pairs_count_.fetch_sub(1);
					m_command_pairs_generated_.pop();
					s_command_pair_pool.deallocate(queued);
					m_command_pairs_count_.notify_all();
				}
				else if (queued && queued->IsReady())
				{
					queued->Execute(false);
					m_command_pairs_count_.fetch_sub(1);
					m_command_pairs_generated_.pop();
					s_command_pair_pool.deallocate(queued);
					m_command_pairs_count_.notify_all();
				}
			}
			else
			{
				m_command_pairs_count_.wait(count);
			}
		}
	}

	void D3Device::Signal(const eCommandTypes type, UINT64 buffer_idx) const
	{
		if (buffer_idx == -1)
		{
			buffer_idx = m_frame_idx_;
		}

		DX::ThrowIfFailed(m_command_queues_[type]->Signal(m_fence_.Get(), ++m_fence_nonce_[buffer_idx]));
	}

	UINT64 D3Device::GetFenceValue(UINT64 buffer_idx) const
	{
		if (buffer_idx == -1)
		{
			buffer_idx = m_frame_idx_;
		}

		return m_fence_nonce_[buffer_idx];
	}
} // namespace Engine::Manager::Graphics

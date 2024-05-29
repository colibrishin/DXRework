#include "pch.h"

#include "egD3Device.hpp"

#include "egDebugger.hpp"
#include "egGlobal.h"
#include "egShader.hpp"
#include "egToolkitAPI.h"

namespace Engine::Manager::Graphics
{
  void D3Device::CopySwapchain(ID3D12Resource* buffer, ID3D12GraphicsCommandList1* command_list) const
  {
    {
      CommandGuard cg;
      command_list->CopyResource(buffer, m_render_targets_[m_frame_idx_].Get());
    }
  }

  HANDLE D3Device::GetSwapchainAwaiter() const { return m_swap_chain_->GetFrameLatencyWaitableObject(); }

  CD3DX12_CPU_DESCRIPTOR_HANDLE D3Device::GetRTVHandle() const
  {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE
      (
       m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart()
      );
  }

  CD3DX12_CPU_DESCRIPTOR_HANDLE D3Device::GetDSVHandle() const
  {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE
      (
       m_dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart()
      );
  }

  D3D12_FEATURE_DATA_ROOT_SIGNATURE D3Device::GetRootSignatureFeature() const
  {
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature
    {
      .HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1
    };

    if (FAILED(m_device_->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature, sizeof(feature))))
    {
      feature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    return feature;
  }

  ID3D12GraphicsCommandList1* D3Device::GetCommandList() const
  {
    return m_command_list_.Get();
  }

  void D3Device::ForceExecuteCommandList() const
  {
    const std::vector<ID3D12CommandList*> command_lists = { m_command_list_.Get() };
    m_command_queue_->ExecuteCommandLists(command_lists.size(), command_lists.data());
  }

  void D3Device::WaitForUploadCompletion()
  {
    ++m_fences_nonce_[m_frame_idx_];
    DX::ThrowIfFailed
    (
      m_command_queue_->Signal
      (
        m_fences_[m_frame_idx_].Get(),
        m_fences_nonce_[m_frame_idx_]
      )
    );
  }

  void D3Device::UpdateBuffer
  (
    const UINT64 size, const void* src, ID3D12Resource* dst
  )
  {
    void*               mapped_data = nullptr;
    const CD3DX12_RANGE range(0, 0);
    DX::ThrowIfFailed(dst->Map(0, &range, &mapped_data));
    memcpy(mapped_data, src, size);
    dst->Unmap(0, nullptr);
  }

  std::vector<std::pair<D3D12_INPUT_ELEMENT_DESC, std::string>> D3Device::GenerateInputDescription(
    ID3DBlob* blob
  )
  {
    ComPtr<ID3D12ShaderReflection> reflection = nullptr;

    DX::ThrowIfFailed(D3DReflect
      (
       blob->GetBufferPointer(), blob->GetBufferSize(),
       IID_ID3D11ShaderReflection,
       reinterpret_cast<void**>(reflection.GetAddressOf())
      ));

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
        if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) { input_desc.Format = DXGI_FORMAT_R32_UINT; }
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
        if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) { input_desc.Format = DXGI_FORMAT_R32G32_UINT; }
        else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
        {
          input_desc.Format = DXGI_FORMAT_R32G32_SINT;
        }
        else if (param_desc.ComponentType ==
                 D3D_REGISTER_COMPONENT_FLOAT32) { input_desc.Format = DXGI_FORMAT_R32G32_FLOAT; }
        byteOffset += 8;
      }
      else if (param_desc.Mask <= 7)
      {
        if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
        {
          input_desc.Format = DXGI_FORMAT_R32G32B32_UINT;
        }
        else if (param_desc.ComponentType ==
                 D3D_REGISTER_COMPONENT_SINT32) { input_desc.Format = DXGI_FORMAT_R32G32B32_SINT; }
        else if (param_desc.ComponentType ==
                 D3D_REGISTER_COMPONENT_FLOAT32) { input_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT; }
        byteOffset += 12;
      }
      else if (param_desc.Mask <= 15)
      {
        if (param_desc.ComponentType ==
            D3D_REGISTER_COMPONENT_UINT32) { input_desc.Format = DXGI_FORMAT_R32G32B32A32_UINT; }
        else if (param_desc.ComponentType ==
                 D3D_REGISTER_COMPONENT_SINT32) { input_desc.Format = DXGI_FORMAT_R32G32B32A32_SINT; }
        else if (param_desc.ComponentType ==
                 D3D_REGISTER_COMPONENT_FLOAT32) { input_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; }
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

  void D3Device::CreateSampler(const D3D12_SAMPLER_DESC& description, const D3D12_CPU_DESCRIPTOR_HANDLE& handle) const
  {
    m_device_->CreateSampler(&description, handle);
  }

  void D3Device::InitializeDevice()
  {
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debug_interface;
    DX::ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
    debug_interface->EnableDebugLayer();
#endif

    // Create factory and Searching for adapter
    ComPtr<IDXGIFactory4> dxgi_factory;

    UINT dxgi_factory_flags = 0;
#ifdef _DEBUG
    dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    DX::ThrowIfFailed(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));

    ComPtr<IDXGIAdapter1> adapter;
    int adapter_idx = 0;

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

#ifdef _DEBUG
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

    if (!adapter)
    {
      throw std::runtime_error("Failed to find a suitable adapter.");
    }

    // If an acceptable adapter is found, create a device and a command queue.
    const D3D12_COMMAND_QUEUE_DESC queue_desc
    {
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT, // All command, compute only, copy only
      .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, // Queue priority between multiple queues.
      .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
    (
     m_device_->CreateCommandQueue
     (
      &queue_desc,
      IID_PPV_ARGS(&m_command_queue_)
     )
    );

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};

    swap_chain_desc.BufferCount        = 2;
    swap_chain_desc.Width              = g_window_width;
    swap_chain_desc.Height             = g_window_height;
    swap_chain_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SampleDesc.Count   = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc.Scaling            = DXGI_SCALING_NONE;
    swap_chain_desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swap_chain_desc.Flags              = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC full_screen_desc = {};

    full_screen_desc.Windowed         = !g_full_screen;
    full_screen_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

    if (g_vsync_enabled)
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
        m_device_.Get(), m_hwnd_, &swap_chain_desc, &full_screen_desc,
        nullptr,
        (IDXGISwapChain1**)m_swap_chain_.GetAddressOf()
       )
      );

    DX::ThrowIfFailed(m_swap_chain_->SetMaximumFrameLatency(g_max_frame_latency_second));
    m_frame_idx_ = m_swap_chain_->GetCurrentBackBufferIndex();

    constexpr D3D12_DESCRIPTOR_HEAP_DESC buffer_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
    (
      m_device_->CreateDescriptorHeap
      (
        &buffer_desc,
        IID_PPV_ARGS(m_buffer_descriptor_heap_.GetAddressOf())
      )
    );
  }

  void D3Device::InitializeRenderTargetView()
  {
    // RTV Descriptor heap
    const D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
      .NumDescriptors = g_frame_buffer,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
    (
      m_device_->CreateDescriptorHeap
      (
        &rtv_heap_desc,
        IID_PPV_ARGS(m_rtv_descriptor_heap_.GetAddressOf())
      )
    );

    m_rtv_desc_size_ = m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto rtv_handle = GetRTVHandle();
    
    m_render_targets_.resize(g_frame_buffer);

    for (int i = 0; i < g_frame_buffer; ++i)
    {
      DX::ThrowIfFailed
      (
        m_swap_chain_->GetBuffer(i, IID_PPV_ARGS(&m_render_targets_[i]))
      );

      m_device_->CreateRenderTargetView(m_render_targets_[i].Get(), nullptr, rtv_handle);

      rtv_handle.Offset(1, m_rtv_desc_size_);
    }
  }

  void D3Device::InitializeCommandAllocator()
  {
    for (int i = 0; i < g_frame_buffer; ++i)
    {
      DX::ThrowIfFailed
      (
        m_device_->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT,
          IID_PPV_ARGS(m_command_allocator_[i].GetAddressOf())
          )
      );
    }

    DX::ThrowIfFailed
    (
      m_device_->CreateCommandList
      (
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_command_allocator_[m_frame_idx_].Get(),
        nullptr,
        IID_PPV_ARGS(m_command_list_.GetAddressOf())
        )
    );

    DX::ThrowIfFailed(m_command_list_->Close());
  }

  void D3Device::InitializeFence()
  {
    for (int i = 0; i < g_frame_buffer; ++i)
    {
      DX::ThrowIfFailed
      (
        m_device_->CreateFence
        (
          0, D3D12_FENCE_FLAG_NONE,
          IID_PPV_ARGS(m_fences_[i].GetAddressOf())
        )
      );

      m_fences_nonce_[i] = 0;
    }

    m_fence_event_ = CreateEvent(nullptr, false, false, nullptr);
    if (m_fence_event_ == nullptr)
    {
      DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
  }

  void D3Device::InitializeD2D()
  {
    ComPtr<ID2D1Factory> d2d_factory;
    DX::ThrowIfFailed
      (
       D2D1CreateFactory
       (
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        d2d_factory.GetAddressOf()
       )
      );

    const auto dpiX =
      static_cast<float>(GetDeviceCaps(GetDC(m_hwnd_), LOGPIXELSX));
    const auto dpiY =
      static_cast<float>(GetDeviceCaps(GetDC(m_hwnd_), LOGPIXELSY));

    const D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties
      (
       D2D1_RENDER_TARGET_TYPE_DEFAULT,
       D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
       dpiX, dpiY
      );

    DX::ThrowIfFailed
      (
       d2d_factory->CreateDxgiSurfaceRenderTarget
       (
        m_surface_.Get(), &props, m_d2d_render_target_view_.GetAddressOf()
       )
      );
  }

  void D3Device::InitializeDepthStencil()
  {
    ComPtr<ID3D12Resource> depth_stencil_buffer;
    const auto             heap_prop     = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const auto             resource_desc = CD3DX12_RESOURCE_DESC::Tex2D
      (
       DXGI_FORMAT_D24_UNORM_S8_UINT, g_window_width, g_window_height, 1, 0, 1, 0,
       D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
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
       m_device_->CreateDescriptorHeap
       (
        &descriptor_heap_desc,
        IID_PPV_ARGS(m_dsv_descriptor_heap_.GetAddressOf())
       )
      );
    
    DX::ThrowIfFailed
      (
       m_device_->CreateCommittedResource
       (
        &heap_prop,
        D3D12_HEAP_FLAG_NONE,
        &resource_desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        nullptr,
        IID_PPV_ARGS(depth_stencil_buffer.ReleaseAndGetAddressOf())
       )
      );

    const auto rtv_handle = GetRTVHandle();
    const auto dsv_handle = GetDSVHandle();
    
    D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
    depth_stencil_view_desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_view_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
    depth_stencil_view_desc.Texture2D.MipSlice = 0;

    m_device_->CreateDepthStencilView
     (
      depth_stencil_buffer.Get(), &depth_stencil_view_desc,
      dsv_handle
     );

    {
      ForceCommandExecutionGuard fcg;
      m_command_list_->OMSetRenderTargets
      (
        1,
        &rtv_handle,
        false,
        &dsv_handle
      );

      m_command_list_->ClearDepthStencilView
      (
        dsv_handle,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f,
        0,
        0,
        nullptr
      );
    }
  }

  void D3Device::WaitForSingleCompletion()
  {
    DX::ThrowIfFailed
    (
      m_command_queue_->Signal
      (
        m_fences_[m_frame_idx_].Get(),
        m_fences_nonce_[m_frame_idx_]
      )
    );

    ++m_fences_nonce_[m_frame_idx_];
  }

  void D3Device::WaitForPreviousFrame()
  {
    if (WaitForSingleObjectEx
        (
         GetSwapchainAwaiter(), g_max_frame_latency_ms,
         true
        ) != WAIT_OBJECT_0)
    {
      GetDebugger().Log("Waiting for Swap chain had an issue.");
    }
    
    const auto backbuffer = m_swap_chain_->GetCurrentBackBufferIndex();

    if (m_fences_[backbuffer]->GetCompletedValue() < m_fences_nonce_[backbuffer])
    {
      DX::ThrowIfFailed
      (
        m_fences_[backbuffer]->SetEventOnCompletion
        (
          m_fences_nonce_[backbuffer],
          m_fence_event_
        )
      );

      WaitForSingleObject(m_fence_event_, INFINITE);
    }

    ++m_fences_nonce_[backbuffer];
  }

  void D3Device::CreateShaderResourceView(
    ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srv, ID3D11ShaderResourceView** view
  ) const
  {
    const auto rtv_handle = GetRTVHandle();
    m_device_->CreateShaderResourceView(resource, &srv, rtv_handle);
  }

  void D3Device::PreUpdate(const float& dt) {}

  void D3Device::Update(const float& dt) {}

  void D3Device::PreRender(const float& dt) { FrameBegin(); }

  void D3Device::Render(const float& dt) {}

  void D3Device::FixedUpdate(const float& dt) {}

  void D3Device::PostRender(const float& dt) { Present(); }

  void D3Device::PostUpdate(const float& dt) {}

  void D3Device::Initialize(HWND hwnd)
  {
    m_hwnd_ = hwnd;

    InitializeDevice();
    InitializeRenderTargetView();
    InitializeCommandAllocator();
    InitializeFence();
    InitializeDepthStencil();

    m_projection_matrix_ = XMMatrixPerspectiveFovLH
      (
       g_fov, GetAspectRatio(),
       g_screen_near, g_screen_far
      );
    m_ortho_matrix_ = XMMatrixOrthographicLH
      (
       g_window_width,
       g_window_height,
       g_screen_near, g_screen_far
      );
  }

  float D3Device::GetAspectRatio()
  {
    return static_cast<float>(g_window_width) /
           static_cast<float>(g_window_height);
  }

  void D3Device::UpdateRenderTarget() const
  {
    const auto& rtv_handle = GetRTVHandle();
    const auto& dsv_handle = GetDSVHandle();

    {
      CommandGuard cg;
      m_command_list_->OMSetRenderTargets(1, &rtv_handle, false, &dsv_handle);
    }
  }

  void D3Device::FrameBegin()
  {
    WaitForPreviousFrame();

    DX::ThrowIfFailed
    (
      m_command_allocator_[m_frame_idx_]->Reset()
    );

    DX::ThrowIfFailed
    (
      m_command_list_->Reset(m_command_allocator_[m_frame_idx_].Get(), nullptr)
    );

    {
      CommandGuard cg;
      constexpr float color[4]   = {0.f, 0.f, 0.f, 1.f};
      const auto&      rtv_handle = GetRTVHandle();
      const auto&      dsv_handle = GetDSVHandle();
    
      const auto initial_barrier = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_render_targets_[m_frame_idx_].Get(),
         D3D12_RESOURCE_STATE_PRESENT,
         D3D12_RESOURCE_STATE_RENDER_TARGET
        );

      m_command_list_->ResourceBarrier(1, &initial_barrier);
      UpdateRenderTarget();
      m_command_list_->ClearRenderTargetView(rtv_handle, color, 0, nullptr);
      m_command_list_->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    
      const auto revert_barrier = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_render_targets_[m_frame_idx_].Get(),
         D3D12_RESOURCE_STATE_RENDER_TARGET,
         D3D12_RESOURCE_STATE_PRESENT
        );

      m_command_list_->ResourceBarrier(1, &revert_barrier);
    }
  }

  void D3Device::Present() const
  {
    const std::vector<ID3D12CommandList*> command_lists = { m_command_list_.Get() };

    m_command_queue_->ExecuteCommandLists(command_lists.size(), command_lists.data());

    DX::ThrowIfFailed
    (
      m_command_queue_->Signal
      (
        m_fences_[m_frame_idx_].Get(),
        m_fences_nonce_[m_frame_idx_]
      )
    );
    
    DXGI_PRESENT_PARAMETERS params;
    params.DirtyRectsCount = 0;
    params.pDirtyRects     = nullptr;
    params.pScrollRect     = nullptr;
    params.pScrollOffset   = nullptr;

    DX::ThrowIfFailed
    (
      m_swap_chain_->Present1
      (
       g_vsync_enabled ? 1 : 0, DXGI_PRESENT_DO_NOT_WAIT,
       &params
      )
    );
  }
} // namespace Engine::Manager::Graphics

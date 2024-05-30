#include "pch.h"

#include "egD3Device.hpp"

#include "egDebugger.hpp"
#include "egGlobal.h"
#include "egShader.hpp"
#include "egStructuredBuffer.hpp"
#include "egToolkitAPI.h"

namespace Engine::Manager::Graphics
{
  HANDLE D3Device::GetSwapchainAwaiter() const { return m_swap_chain_->GetFrameLatencyWaitableObject(); }

  ID3D12GraphicsCommandList1* D3Device::GetCommandList() const
  {
    return m_direct_command_list_.Get();
  }

  ID3D12GraphicsCommandList1* D3Device::GetCopyCommandList() const
  {
    return m_copy_command_list_.Get();
  }

  ID3D12GraphicsCommandList1* D3Device::GetComputeCommandList() const
  {
    return m_compute_command_list_.Get();
  }

  void D3Device::WaitForUploadCompletion()
  {
    Signal(m_frame_idx_);
  }

  void D3Device::ExecuteDirectCommandList() const
  {
    const std::vector<ID3D12CommandList*> command_lists
    {
      GetD3Device().GetCommandList()
    };

    m_direct_command_queue_->ExecuteCommandLists(command_lists.size(), command_lists.data());
  }

  void D3Device::ExecuteCopyCommandList()
  {
    const std::vector<ID3D12CommandList*> command_lists = { m_copy_command_list_.Get() };
    m_copy_command_queue_->ExecuteCommandLists(command_lists.size(), command_lists.data());

    DX::ThrowIfFailed(m_copy_command_queue_->Signal(m_fence_.Get(), ++m_fence_nonce_[m_frame_idx_]));

    WaitForEventCompletion(m_frame_idx_);

    DX::ThrowIfFailed(m_copy_command_list_->Reset(m_command_allocator_[m_frame_idx_].Get(), nullptr));
  }

  void D3Device::ExecuteComputeCommandList()
  {
    const std::vector<ID3D12CommandList*> command_lists = { m_compute_command_list_.Get() };
    m_compute_command_queue_->ExecuteCommandLists(command_lists.size(), command_lists.data());

    DX::ThrowIfFailed(m_compute_command_queue_->Signal(m_fence_.Get(), ++m_fence_nonce_[m_frame_idx_]));

    WaitForEventCompletion(m_frame_idx_);

    DX::ThrowIfFailed(m_compute_command_list_->Reset(m_command_allocator_[m_frame_idx_].Get(), nullptr));
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

  void D3Device::CreateSampler(const D3D12_SAMPLER_DESC& description, const D3D12_CPU_DESCRIPTOR_HANDLE& sampler_handle) const
  {
    m_device_->CreateSampler(&description, sampler_handle);
  }

  void D3Device::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& description) const
  {
    m_device_->CreateConstantBufferView(
        &description, 
        GetRenderPipeline().GetCBHeap()->GetCPUDescriptorHandleForHeapStart());
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
    constexpr D3D12_COMMAND_QUEUE_DESC direct_desc
    {
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT, // All command, compute only, copy only
      .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, // Queue priority between multiple queues.
      .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
      .NodeMask = 0
    };

    constexpr D3D12_COMMAND_QUEUE_DESC copy_desc
    {
      .Type = D3D12_COMMAND_LIST_TYPE_COPY,
      .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
      .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
      .NodeMask = 0
    };

    constexpr D3D12_COMMAND_QUEUE_DESC compute_desc
    {
      .Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
      .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
      .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
    (
     m_device_->CreateCommandQueue
     (
      &direct_desc,
      IID_PPV_ARGS(&m_direct_command_queue_)
     )
    );

    DX::ThrowIfFailed
      (
       m_device_->CreateCommandQueue
       (
        &copy_desc,
        IID_PPV_ARGS(&m_copy_command_queue_)
       )
      );

    DX::ThrowIfFailed
      (
       m_device_->CreateCommandQueue
       (
        &compute_desc,
        IID_PPV_ARGS(&m_compute_command_queue_)
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
        m_command_queues_[COMMAND_IDX_DIRECT].Get(), m_hwnd_, &swap_chain_desc, &full_screen_desc,
        nullptr,
        (IDXGISwapChain1**)m_swap_chain_.GetAddressOf()
       )
      );

    DX::ThrowIfFailed(m_swap_chain_->SetMaximumFrameLatency(g_max_frame_latency_second));
    m_frame_idx_ = m_swap_chain_->GetCurrentBackBufferIndex();
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
        IID_PPV_ARGS(m_direct_command_list_.GetAddressOf())
        )
    );

    DX::ThrowIfFailed
      (
       m_device_->CreateCommandList
       (
        0,
        D3D12_COMMAND_LIST_TYPE_COPY,
        m_command_allocator_[m_frame_idx_].Get(),
        nullptr,
        IID_PPV_ARGS(m_copy_command_list_.GetAddressOf())
       )
      );

    DX::ThrowIfFailed
      (
       m_device_->CreateCommandList
       (
        0,
        D3D12_COMMAND_LIST_TYPE_COMPUTE,
        m_command_allocator_[m_frame_idx_].Get(),
        nullptr,
        IID_PPV_ARGS(m_compute_command_list_.GetAddressOf())
       )
      );

    DX::ThrowIfFailed(m_direct_command_list_->Close());
    DX::ThrowIfFailed(m_copy_command_list_->Close());
    DX::ThrowIfFailed(m_compute_command_list_->Close());
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

    m_fence_nonce_.resize(g_frame_buffer, 0);

    m_fence_event_ = CreateEvent(nullptr, false, false, nullptr);

    if (m_fence_event_ == nullptr)
    {
      DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
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

  void D3Device::Signal(const UINT64 buffer_idx)
  {
    DX::ThrowIfFailed
    (
      m_direct_command_queue_->Signal
      (
        m_fence_.Get(),
        ++m_fence_nonce_[buffer_idx]
      )
    );
  }

  void D3Device::WaitForBackBuffer() const
  {
    // Buffer for next frame.
    const auto& back_buffer_idx = m_swap_chain_->GetCurrentBackBufferIndex();

    WaitForEventCompletion(back_buffer_idx);
  }

  void D3Device::PreUpdate(const float& dt) { FrameBegin(); }

  void D3Device::Update(const float& dt) {}

  void D3Device::PreRender(const float& dt) {}

  void D3Device::Render(const float& dt) { WaitNextFrame(); }

  ID3D12CommandAllocator* D3Device::GetCopyCommandAllocator(UINT frame_idx) const
  {
    if (frame_idx == -1)
    {
      frame_idx = m_frame_idx_;
    }

  void D3Device::PostRender(const float& dt) {}

  ID3D12CommandAllocator* D3Device::GetComputeCommandAllocator(UINT frame_idx) const
  {
    if (frame_idx == -1)
    {
      frame_idx = m_frame_idx_;
    }

    return m_command_allocators_.at(frame_idx)[COMMAND_IDX_COMPUTE].Get();
  }

  ID3D12CommandAllocator* D3Device::GetSubDirectCommandAllocator(UINT frame_idx) const
  {
    if (frame_idx == -1)
    {
      frame_idx = m_frame_idx_;
    }

    InitializeDevice();
    InitializeCommandAllocator();
    InitializeFence();

  void D3Device::Reset(const eCommandListIndex type, UINT64 buffer_idx) const
  {
    if (buffer_idx == -1)
    {
      buffer_idx = m_frame_idx_;
    }

    DX::ThrowIfFailed
      (
       m_command_lists_.at(buffer_idx)[type]->Reset
       (
        m_command_allocators_.at(buffer_idx)[type].Get(),
        nullptr
       )
      );
    m_ortho_matrix_ = XMMatrixOrthographicLH
      (
       g_window_width,
       g_window_height,
       g_screen_near, g_screen_far
      );
  }

  void D3Device::Signal(const eCommandListIndex type, UINT64 buffer_idx)
  {
    if (buffer_idx == -1)
    {
      buffer_idx = m_frame_idx_;
    }

    switch (type)
    {
      case COMMAND_IDX_DIRECT:
        DX::ThrowIfFailed
          (
           GetDirectCommandQueue()->Signal
           (
            m_fence_.Get(),
            ++m_fence_nonce_[buffer_idx]
           )
          );
        break;
      case COMMAND_IDX_COPY:
        DX::ThrowIfFailed
          (
           GetCopyCommandQueue()->Signal
           (
            m_fence_.Get(),
            ++m_fence_nonce_[buffer_idx]
           )
          );
        break;
      case COMMAND_IDX_COMPUTE:
        DX::ThrowIfFailed
          (
           GetComputeCommandQueue()->Signal
           (
            m_fence_.Get(),
            ++m_fence_nonce_[buffer_idx]
           )
          );
        break;
      case COMMAND_IDX_SUB_DIRECT: 
          DX::ThrowIfFailed
          (
           GetSubDirectCommandQueue()->Signal
           (
            m_fence_.Get(),
            ++m_fence_nonce_[buffer_idx]
           )
          );
        break;
      default: break;
    }
  }

  void D3Device::FrameBegin() const
  {

    DX::ThrowIfFailed
    (
      m_command_allocator_[m_frame_idx_]->Reset()
    );

    DX::ThrowIfFailed
    (
      m_direct_command_list_->Reset(m_command_allocator_[m_frame_idx_].Get(), nullptr)
    );

    DX::ThrowIfFailed
      (
       m_copy_command_list_->Reset(m_command_allocator_[m_frame_idx_].Get(), nullptr)
      );

    DX::ThrowIfFailed
      (
       m_compute_command_list_->Reset(m_command_allocator_[m_frame_idx_].Get(), nullptr)
      );
  }

  void D3Device::WaitNextFrame()
  {
    // Signaling the next frame.
    m_frame_idx_ = m_swap_chain_->GetCurrentBackBufferIndex();

    Signal(m_frame_idx_);
    WaitForBackBuffer();
  }
} // namespace Engine::Manager::Graphics

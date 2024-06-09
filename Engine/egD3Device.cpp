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

  ID3D12GraphicsCommandList1* D3Device::GetDirectCommandList(UINT frame_idx) const
  {
    if (frame_idx == -1)
    {
      frame_idx = m_frame_idx_;
    }

    return m_command_lists_.at(m_frame_idx_)[COMMAND_IDX_DIRECT].Get();
  }

  ID3D12GraphicsCommandList1* D3Device::GetCopyCommandList(UINT frame_idx) const
  {
    if (frame_idx == -1)
    {
      frame_idx = m_frame_idx_;
    }

    return m_command_lists_.at(m_frame_idx_)[COMMAND_IDX_COPY].Get();
  }

  ID3D12GraphicsCommandList1* D3Device::GetComputeCommandList(UINT frame_idx) const
  {
    if (frame_idx == -1)
    {
      frame_idx = m_frame_idx_;
    }

    return m_command_lists_.at(m_frame_idx_)[COMMAND_IDX_COMPUTE].Get();
  }

  ID3D12GraphicsCommandList1* D3Device::GetToolkitCommandList(UINT frame_idx) const
  {
    if (frame_idx == -1)
    {
      frame_idx = m_frame_idx_;
    }

    return m_command_lists_.at(m_frame_idx_)[COMMAND_IDX_TOOLKIT].Get();
  }

  void D3Device::ExecuteDirectCommandList()
  {
    DX::ThrowIfFailed(GetDirectCommandList()->Close());

    const std::vector<ID3D12CommandList*> command_lists
    {
      GetD3Device().GetDirectCommandList()
    };

    GetDirectCommandQueue()->ExecuteCommandLists(command_lists.size(), command_lists.data());
    Signal(COMMAND_PRIMITIVE_DIRECT);
  }

  void D3Device::ExecuteCopyCommandList()
  {
    DX::ThrowIfFailed(GetCopyCommandList()->Close());

    const std::vector<ID3D12CommandList*> command_lists
    {
      GetCopyCommandList()
    };

    GetCopyCommandQueue()->ExecuteCommandLists(command_lists.size(), command_lists.data());
    Signal(COMMAND_PRIMITIVE_COPY);
  }

  void D3Device::ExecuteComputeCommandList()
  {
    DX::ThrowIfFailed(GetComputeCommandList()->Close());

    const std::vector<ID3D12CommandList*> command_lists
    {
      GetComputeCommandList()
    };

    GetComputeCommandQueue()->ExecuteCommandLists(command_lists.size(), command_lists.data());
    Signal(COMMAND_PRIMITIVE_COMPUTE);
  }

  void D3Device::ExecuteToolkitCommandList()
  {
    DX::ThrowIfFailed(GetToolkitCommandList()->Close());

    const std::vector<ID3D12CommandList*> command_lists
    {
      GetToolkitCommandList()
    };

    GetDirectCommandQueue()->ExecuteCommandLists(command_lists.size(), command_lists.data());
    Signal(COMMAND_PRIMITIVE_DIRECT);
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

  void D3Device::CreateConstantBufferView(const UINT slot, const D3D12_CONSTANT_BUFFER_VIEW_DESC& description) const
  {
    const auto& cbv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
      (
       GetRenderPipeline().GetBufferHeap()->GetCPUDescriptorHandleForHeapStart(),
       slot,
       m_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
      );

    m_device_->CreateConstantBufferView(&description, cbv_handle);
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
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
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

    for (int i = 0; i < COMMAND_PRIMITIVE_COUNT; ++i)
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
  }

  void D3Device::InitializeCommandAllocator()
  {
    for (int i = 0; i < g_frame_buffer; ++i)
    {
      for (int t = 0; t < _countof(s_target_types); ++t)
      {
        DX::ThrowIfFailed
        (
          m_device_->CreateCommandAllocator
          (
            static_cast<D3D12_COMMAND_LIST_TYPE>(s_target_types[i]),
            IID_PPV_ARGS(m_command_allocators_[i][t].GetAddressOf())
          )
        );

        DX::ThrowIfFailed
          (
           m_device_->CreateCommandList
           (
            0,
            static_cast<D3D12_COMMAND_LIST_TYPE>(s_target_types[i]),
            m_command_allocators_[m_frame_idx_][t].Get(),
            nullptr,
            IID_PPV_ARGS(m_command_lists_[i][t].GetAddressOf())
           )
          );

        DX::ThrowIfFailed
          (
           m_command_lists_[i][t]->Close()
          );
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

    m_fence_nonce_ = new std::atomic<UINT64>[g_frame_buffer]{0, };

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

  void D3Device::WaitForBackBuffer() const
  {
    // Buffer for next frame.
    const auto& back_buffer_idx = m_swap_chain_->GetCurrentBackBufferIndex();

    WaitForEventCompletion(back_buffer_idx);
  }

  void D3Device::PreUpdate(const float& dt) { }

  void D3Device::Update(const float& dt) {}

  void D3Device::PreRender(const float& dt) {}

  void D3Device::Render(const float& dt) {}

  void D3Device::FixedUpdate(const float& dt) {}

  void D3Device::PostRender(const float& dt) {}

  void D3Device::PostUpdate(const float& dt) {}

  void D3Device::Initialize(HWND hwnd)
  {
    m_hwnd_ = hwnd;

    InitializeDevice();
    InitializeCommandAllocator();
    InitializeFence();

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

  void D3Device::CreateTextureFromFile(
      const std::filesystem::path& file_path,
      ID3D12Resource** res,
      bool generate_mip = false) const
  {
    if (!std::filesystem::exists(file_path))
    {
      throw std::runtime_error("File not found.");
    }

    ResourceUploadBatch resource_upload_batch(m_device_.Get());

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

    const auto& token = resource_upload_batch.End(GetCopyCommandQueue());
    token.wait();
  }

  float D3Device::GetAspectRatio()
  {
    return static_cast<float>(g_window_width) /
           static_cast<float>(g_window_height);
  }

  void D3Device::CleanupCommandList()
  {
    for (UINT64 i = 0; i < g_frame_buffer; ++i)
    {
      for (int t = 0; t < _countof(s_target_types); ++t)
      {
        DX::ThrowIfFailed(m_command_allocators_.at(i)[t].Reset());
      }
    }

    for (int i = 0; i < _countof(s_target_types); ++i)
    {
      DX::ThrowIfFailed
      (
        m_command_lists_.at(m_frame_idx_)[i]->Reset
        (
          m_command_allocators_.at(m_frame_idx_)[i].Get(),
          nullptr
        )
      );
    }
  }

  void D3Device::WaitAndReset(const eCommandListIndex type, UINT64 buffer_idx) const
  {
    if (buffer_idx == -1)
    {
      buffer_idx = m_frame_idx_;
    }

    WaitForEventCompletion(buffer_idx);
    Reset(type, buffer_idx);
  }

  void D3Device::WaitNextFrame()
  {
    // Signaling the next frame.
    m_frame_idx_ = m_swap_chain_->GetCurrentBackBufferIndex();

    Signal(COMMAND_PRIMITIVE_DIRECT, m_frame_idx_);
    WaitForBackBuffer();
  }

  ID3D12CommandAllocator* D3Device::GetDirectCommandAllocator(UINT frame_idx) const
  {
    if (frame_idx == -1)
    {
      frame_idx = m_frame_idx_;
    }

    return m_command_allocators_.at(frame_idx)[COMMAND_IDX_DIRECT].Get();
  }

  ID3D12CommandAllocator* D3Device::GetCopyCommandAllocator(UINT frame_idx) const
  {
    if (frame_idx == -1)
    {
      frame_idx = m_frame_idx_;
    }

    return m_command_allocators_.at(frame_idx)[COMMAND_IDX_COPY].Get();
  }

  ID3D12CommandAllocator* D3Device::GetComputeCommandAllocator(UINT frame_idx) const
  {
    if (frame_idx == -1)
    {
      frame_idx = m_frame_idx_;
    }

    return m_command_allocators_.at(frame_idx)[COMMAND_IDX_COMPUTE].Get();
  }

  ID3D12CommandAllocator* D3Device::GetToolkitCommandAllocator(UINT frame_idx) const
  {
    if (frame_idx == -1)
    {
      frame_idx = m_frame_idx_;
    }

    return m_command_allocators_.at(frame_idx)[COMMAND_IDX_TOOLKIT].Get();
  }

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
  }

  void D3Device::Signal(const eCommandPrimitiveType type, UINT64 buffer_idx)
  {
    if (buffer_idx == -1)
    {
      buffer_idx = m_frame_idx_;
    }

    switch (type)
    {
      case COMMAND_PRIMITIVE_DIRECT:
        DX::ThrowIfFailed
          (
           GetDirectCommandQueue()->Signal
           (
            m_fence_.Get(),
            ++m_fence_nonce_[buffer_idx]
           )
          );
        break;
      case COMMAND_PRIMITIVE_COPY:
        DX::ThrowIfFailed
          (
           GetCopyCommandQueue()->Signal
           (
            m_fence_.Get(),
            ++m_fence_nonce_[buffer_idx]
           )
          );
        break;
      case COMMAND_PRIMITIVE_COMPUTE:
        DX::ThrowIfFailed
          (
           GetComputeCommandQueue()->Signal
           (
            m_fence_.Get(),
            ++m_fence_nonce_[buffer_idx]
           )
          );
        break;
      default:
        break;
    }
  }

  ID3D12CommandQueue* D3Device::GetDirectCommandQueue() const
  {
    return m_command_queues_[COMMAND_PRIMITIVE_DIRECT].Get();
  }

  ID3D12CommandQueue* D3Device::GetCopyCommandQueue() const
  {
    return m_command_queues_[COMMAND_PRIMITIVE_COPY].Get();
  }

  ID3D12CommandQueue* D3Device::GetComputeCommandQueue() const
  {
    return m_command_queues_[COMMAND_PRIMITIVE_COMPUTE].Get();
  }
} // namespace Engine::Manager::Graphics

#include "pch.h"
#include "egRenderPipeline.h"

#include <filesystem>

#include "egConstantBuffer.hpp"
#include "egD3Device.hpp"
#include "egManagerHelper.hpp"
#include "egShader.hpp"
#include "egToolkitAPI.h"
#include "egType.h"

namespace Engine::Manager::Graphics
{
  using namespace Engine::Resources;

  void RenderPipeline::SetWorldMatrix(const CBs::TransformCB& matrix)
  {
    m_transform_buffer_ = matrix;
  }

  void RenderPipeline::SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix)
  {
    m_wvp_buffer_ = matrix;
  }

  void RenderPipeline::DefaultRenderTarget() const
  {
    const auto& rtv_handle = m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
    const auto& dsv_handle = m_dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();

    GetD3Device().GetDirectCommandList()->OMSetRenderTargets(1, &rtv_handle, false, &dsv_handle);
  }

  void RenderPipeline::DefaultViewport() const
  {
    GetD3Device().GetDirectCommandList()->RSSetViewports(1, &m_viewport_);
  }
  
  RenderPipeline::~RenderPipeline() { }

  void RenderPipeline::InitializeStaticBuffers()
  {
    m_wvp_buffer_data_.Create(nullptr);
    m_transform_buffer_data_.Create(nullptr);
    m_material_buffer_data_.Create(nullptr);
    m_param_buffer_data_.Create(nullptr);
  }

  void RenderPipeline::SetRootSignature()
  {
    GetD3Device().GetDirectCommandList()->SetGraphicsRootSignature(m_root_signature_.Get());
  }

  void RenderPipeline::SetHeaps()
  {
    ID3D12DescriptorHeap* heaps[]
    {
      m_buffer_descriptor_heap_.Get(),
      m_sampler_descriptor_heap_.Get()
    };

    GetD3Device().GetDirectCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

    GetD3Device().GetDirectCommandList()->SetGraphicsRootDescriptorTable
      (DESCRIPTOR_SLOT_SAMPLER, m_sampler_descriptor_heap_->GetGPUDescriptorHandleForHeapStart());

    GetD3Device().GetDirectCommandList()->SetComputeRootDescriptorTable
      (DESCRIPTOR_SLOT_SAMPLER, m_sampler_descriptor_heap_->GetGPUDescriptorHandleForHeapStart());

    CD3DX12_GPU_DESCRIPTOR_HANDLE buffer_handle
      (
       m_buffer_descriptor_heap_->GetGPUDescriptorHandleForHeapStart()
      );

    // SRV
    GetD3Device().GetDirectCommandList()->SetGraphicsRootDescriptorTable(DESCRIPTOR_SLOT_SRV, buffer_handle);
    GetD3Device().GetDirectCommandList()->SetComputeRootDescriptorTable(DESCRIPTOR_SLOT_SRV, buffer_handle);

  void RenderPipeline::BindVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& view)
  {
    GetD3Device().GetCommandList()->IASetVertexBuffers(0, 1, &view);
  }

  void RenderPipeline::BindIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& view)
  {
    GetD3Device().GetCommandList()->IASetIndexBuffer(&view);
  }

  void RenderPipeline::UnbindVertexBuffer()
  {
    GetD3Device().GetCommandList()->IASetVertexBuffers(0, 0, nullptr);
  }

  void RenderPipeline::UnbindIndexBuffer()
  {
    GetD3Device().GetCommandList()->IASetIndexBuffer(nullptr);
  }

  void RenderPipeline::BindResource(
    UINT                       slot,
    eShaderType                shader_type,
    ID3D11ShaderResourceView** texture
  ) { g_shader_rs_bind_map.at(shader_type)(GetD3Device().GetContext(), texture, slot, 1); }

  RenderPipeline::~RenderPipeline() { ResetShaders(); }

  void RenderPipeline::Initialize()
  {
    GetD3Device().WaitAndReset(COMMAND_IDX_DIRECT);

    PrecompileShaders();
    InitializeRootSignature();
    InitializeDefaultPSO();
  }

  void RenderPipeline::PrecompileShaders()
  {
    Shader::Create
      (
       "default", "./default.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "color", "./color.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "skybox", "./skybox.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_NONE | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "specular_normal", "./specular_normal.hlsl",
       SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "normal", "./normal.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "refraction", "./refraction.hlsl", SHADER_DOMAIN_POST_PROCESS,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "specular_tex", "./specular_tex.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "specular", "./specular.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "cascade_shadow_stage1", "./cascade_shadow_stage1.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_CLAMP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    const auto billboard = Shader::Create
      (
       "billboard", "./billboard.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_NONE | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST
      );

    Shader::Create
      (
       "intensity_test", "./intensity_test.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_POINT,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_NEVER, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "atlas", "./atlas.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );
  }

  void RenderPipeline::InitializeDefaultPSO()
  {
    
  }

  void RenderPipeline::InitializeRootSignature()
  {
    CD3DX12_DESCRIPTOR_RANGE1 ranges[4];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, BIND_SLOT_UAV_END, 0);
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BIND_SLOT_END, 0);
    ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, CB_TYPE_END, 0);

    CD3DX12_ROOT_PARAMETER1 root_parameters[3];
    root_parameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
    root_parameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
    root_parameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);
    root_parameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_STATIC_SAMPLER_DESC static_sampler_desc[2];
    static_sampler_desc[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    static_sampler_desc[1].Init(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT);

    const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc
    (
      _countof(root_parameters),
      root_parameters,
      _countof(static_sampler_desc),
      static_sampler_desc
    );

    ComPtr<ID3DBlob> signature_blob = nullptr;
    DX::ThrowIfFailed
      (
       D3D12SerializeVersionedRootSignature
       (
        &root_signature_desc, signature_blob.GetAddressOf(), nullptr
       )
      );

    DX::ThrowIfFailed
    (
      GetD3Device().GetDevice()->CreateRootSignature
      (
        0,
        signature_blob->GetBufferPointer(),
        signature_blob->GetBufferSize(),
        IID_PPV_ARGS(m_root_signature_.ReleaseAndGetAddressOf())
        )
    );
  }

  void RenderPipeline::InitializeRenderTargets()
  {
    m_render_targets_.resize(g_frame_buffer);

    const D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
      .NumDescriptors = g_frame_buffer,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &rtv_heap_desc, IID_PPV_ARGS(m_rtv_descriptor_heap_.ReleaseAndGetAddressOf())
       )
      );

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

    constexpr D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
    {
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
      .Texture2D = { 0, 0 }
    };

    for (UINT i = 0; i < g_frame_buffer; ++i)
    {
      DX::ThrowIfFailed
        (
         GetD3Device().m_swap_chain_->GetBuffer
         (
          i, IID_PPV_ARGS(m_render_targets_[i].ReleaseAndGetAddressOf())
         )
        );

      GetD3Device().GetDevice()->CreateRenderTargetView
        (
         m_render_targets_[i].Get(), &rtv_desc, rtv_handle
        );

      rtv_handle.Offset(1, m_rtv_descriptor_size_);
    }
  }

  void RenderPipeline::InitializeDepthStencil()
  {
    const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const auto& depth_desc   = CD3DX12_RESOURCE_DESC::Tex2D
      (
       DXGI_FORMAT_D24_UNORM_S8_UINT,
       g_window_width,
       g_window_height,
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
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &descriptor_heap_desc, IID_PPV_ARGS(m_dsv_descriptor_heap_.ReleaseAndGetAddressOf())
       )
      );

    constexpr D3D12_CLEAR_VALUE clear_value
    {
      .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
      .DepthStencil = { 1.0f, 0 }
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateCommittedResource
       (
        &default_heap,
        D3D12_HEAP_FLAG_NONE,
        &depth_desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clear_value,
        IID_PPV_ARGS(m_depth_stencil_.ReleaseAndGetAddressOf())
       )
      );

    GetD3Device().GetDevice()->CreateDepthStencilView
      (
       m_depth_stencil_.Get(), nullptr, m_dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart()
      );
  }

  void RenderPipeline::InitializeNullDescriptors()
  {
    // Dummy (null) descriptor
    constexpr D3D12_DESCRIPTOR_HEAP_DESC null_buffer_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0
    };

    constexpr D3D12_DESCRIPTOR_HEAP_DESC null_sampler_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0
    };

    constexpr D3D12_DESCRIPTOR_HEAP_DESC null_rtv_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0
    };

    constexpr D3D12_DESCRIPTOR_HEAP_DESC null_dsv_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &null_buffer_heap_desc,
        IID_PPV_ARGS(m_null_cbv_heap_.ReleaseAndGetAddressOf())
       )
      );

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &null_buffer_heap_desc,
        IID_PPV_ARGS(m_null_srv_heap_.ReleaseAndGetAddressOf())
       )
      );

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &null_buffer_heap_desc,
        IID_PPV_ARGS(m_null_uav_heap_.ReleaseAndGetAddressOf())
       )
      );

    GetD3Device().GetDevice()->CreateConstantBufferView
      (
       nullptr, m_null_cbv_heap_->GetCPUDescriptorHandleForHeapStart()
      );

    constexpr D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc
    {
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
      .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
      .Texture2D = {0, 0, 0, 0}
    };

    GetD3Device().GetDevice()->CreateShaderResourceView
      (
       nullptr, &srv_desc, m_null_srv_heap_->GetCPUDescriptorHandleForHeapStart()
      );

    constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc
    {
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
      .Texture2D = { 0, 0, }
    };

    GetD3Device().GetDevice()->CreateUnorderedAccessView
      (
       nullptr, nullptr, &uav_desc, m_null_uav_heap_->GetCPUDescriptorHandleForHeapStart()
      );

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &null_sampler_heap_desc,
        IID_PPV_ARGS(m_null_sampler_heap_.ReleaseAndGetAddressOf())
       )
      );

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &null_rtv_heap_desc,
        IID_PPV_ARGS(m_null_rtv_heap_.ReleaseAndGetAddressOf())
       )
      );

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &null_dsv_heap_desc,
        IID_PPV_ARGS(m_null_dsv_heap_.ReleaseAndGetAddressOf())
       )
      );

    constexpr D3D12_SAMPLER_DESC sampler_desc
    {
      .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
      .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      .MipLODBias = 0,
      .MaxAnisotropy = 0,
      .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
      .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
      .MinLOD = 0,
      .MaxLOD = D3D12_FLOAT32_MAX
    };

    GetD3Device().GetDevice()->CreateSampler
      (
       &sampler_desc, m_null_sampler_heap_->GetCPUDescriptorHandleForHeapStart()
      );

    constexpr D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
    {
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
      .Texture2D = {0, 0}
    };

    GetD3Device().GetDevice()->CreateRenderTargetView
      (
       nullptr, &rtv_desc, m_null_rtv_heap_->GetCPUDescriptorHandleForHeapStart()
      );

    constexpr D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc
    {
      .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
      .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
      .Flags = D3D12_DSV_FLAG_NONE,
      .Texture2D = {0}
    };

    GetD3Device().GetDevice()->CreateDepthStencilView
      (
       nullptr, &dsv_desc, m_null_dsv_heap_->GetCPUDescriptorHandleForHeapStart()
      );
  }

  void RenderPipeline::InitializeHeaps()
  {
    constexpr D3D12_DESCRIPTOR_HEAP_DESC buffer_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = g_total_engine_slots,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &buffer_heap_desc, IID_PPV_ARGS(m_buffer_descriptor_heap_.ReleaseAndGetAddressOf())
       )
      );

    constexpr D3D12_DESCRIPTOR_HEAP_DESC sampler_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
      .NumDescriptors = g_max_sampler_slots,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &sampler_heap_desc, IID_PPV_ARGS(m_sampler_descriptor_heap_.ReleaseAndGetAddressOf())
       )
      );

    m_buffer_descriptor_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize
      (D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_rtv_descriptor_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    m_dsv_descriptor_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    m_sampler_descriptor_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    InitializeNullDescriptors();
  }

  void RenderPipeline::PreUpdate(const float& dt) {}

  void RenderPipeline::PreRender(const float& dt)
  {
    GetD3Device().WaitNextFrame();

    GetD3Device().WaitAndReset(COMMAND_IDX_DIRECT);

    SetRootSignature();
    SetHeaps();

    constexpr float color[4]   = {0.f, 0.f, 0.f, 1.f};
    const auto&      rtv_handle = m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
    const auto&      dsv_handle = m_dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
  
    const auto initial_barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_render_targets_[GetD3Device().GetFrameIndex()].Get(),
       D3D12_RESOURCE_STATE_PRESENT,
       D3D12_RESOURCE_STATE_RENDER_TARGET
      );

    GetD3Device().GetDirectCommandList()->OMSetRenderTargets(1, &rtv_handle, false, &dsv_handle);

    GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &initial_barrier);

    GetD3Device().GetDirectCommandList()->ClearRenderTargetView(rtv_handle, color, 0, nullptr);
    GetD3Device().GetDirectCommandList()->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    FallbackPSO();
  }

  void RenderPipeline::Update(const float& dt) {}

  void RenderPipeline::Render(const float& dt) {}

  void RenderPipeline::FixedUpdate(const float& dt) {}

  void RenderPipeline::PostRender(const float& dt)
  {
    const auto present_barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_render_targets_[GetD3Device().GetFrameIndex()].Get(),
       D3D12_RESOURCE_STATE_RENDER_TARGET,
       D3D12_RESOURCE_STATE_PRESENT
      );

    GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &present_barrier);

    GetD3Device().ExecuteDirectCommandList();

    DXGI_PRESENT_PARAMETERS params;
    params.DirtyRectsCount = 0;
    params.pDirtyRects     = nullptr;
    params.pScrollRect     = nullptr;
    params.pScrollOffset   = nullptr;

    DX::ThrowIfFailed
      (
       GetD3Device().m_swap_chain_->Present1
       (
        g_vsync_enabled ? 1 : 0, DXGI_PRESENT_DO_NOT_WAIT,
        &params
       )
      );

    if (WaitForSingleObjectEx
        (
         GetD3Device().GetSwapchainAwaiter(), g_max_frame_latency_ms,
         true
        ) != WAIT_OBJECT_0)
    {
      GetDebugger().Log("Waiting for Swap chain had an issue.");
    }
  }

  void RenderPipeline::PostUpdate(const float& dt) {}

  void RenderPipeline::DrawIndexedDeferred(UINT index_count)
  {
    GetD3Device().GetDirectCommandList()->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
  }

  RTVDSVHandlePair RenderPipeline::SetRenderTargetDeferred(const D3D12_CPU_DESCRIPTOR_HANDLE& rtv)
  {
    const auto& current_rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE
      (
       m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
       GetD3Device().GetFrameIndex(),
       m_rtv_descriptor_size_
      );

    const auto& current_dsv = m_dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();

    GetD3Device().GetDirectCommandList()->OMSetRenderTargets(1, &rtv, false, &current_dsv);

    return {current_rtv_handle, current_dsv};
  }

  RTVDSVHandlePair RenderPipeline::SetRenderTargetDeferred(
    const D3D12_CPU_DESCRIPTOR_HANDLE& rtv, const D3D12_CPU_DESCRIPTOR_HANDLE& dsv
  )
  {
    CD3DX12_CPU_DESCRIPTOR_HANDLE current_rtv_handle
      (
       m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
       GetD3Device().GetFrameIndex(),
       m_rtv_descriptor_size_
      );

    CD3DX12_CPU_DESCRIPTOR_HANDLE current_dsv_handle
      (
       m_dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart()
      );

    GetD3Device().GetDirectCommandList()->OMSetRenderTargets(
        1, 
        &rtv, 
        false, 
        &dsv);

    return {current_rtv_handle, current_dsv_handle};
  }

  RTVDSVHandlePair RenderPipeline::SetRenderTargetDeferred(
    const UINT count, const D3D12_CPU_DESCRIPTOR_HANDLE* srv, const D3D12_CPU_DESCRIPTOR_HANDLE& dsv
  )
  {
    CD3DX12_CPU_DESCRIPTOR_HANDLE current_rtv_handle
      (
            m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
                GetD3Device().GetFrameIndex(),
                m_rtv_descriptor_size_
           );

    CD3DX12_CPU_DESCRIPTOR_HANDLE current_dsv_handle
      (
            m_dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart()
           );

    GetD3Device().GetDirectCommandList()->OMSetRenderTargets
      (
       count, srv, false, &dsv
      );

    return {current_rtv_handle, current_dsv_handle};
  }

  void RenderPipeline::SetRenderTargetDeferred(const RTVDSVHandlePair& rtv_dsv_pair) const
  {
    GetD3Device().GetDirectCommandList()->OMSetRenderTargets(1, &rtv_dsv_pair.first, false, &rtv_dsv_pair.second);
  }

  RTVDSVHandlePair RenderPipeline::SetDepthStencilDeferred(const D3D12_CPU_DESCRIPTOR_HANDLE& dsv) const
  {
    const auto& current_rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE
      (
       m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
       GetD3Device().GetFrameIndex(),
       m_rtv_descriptor_size_
      );

    const auto& current_dsv = m_dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();

    GetD3Device().GetDirectCommandList()->OMSetRenderTargets(1, &current_rtv, false, &dsv);

    return {current_rtv, current_dsv};
  }

  RTVDSVHandlePair RenderPipeline::SetDepthStencilOnlyDeferred(const D3D12_CPU_DESCRIPTOR_HANDLE& dsv) const
  {
    const auto& null_rtv = m_null_rtv_heap_->GetCPUDescriptorHandleForHeapStart();

    const auto& current_rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE
      (
       m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
       GetD3Device().GetFrameIndex(),
       m_rtv_descriptor_size_
      );

    const auto& current_dsv = m_dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();

    GetD3Device().GetDirectCommandList()->OMSetRenderTargets(0, &null_rtv, false, &dsv);

    return {current_rtv, current_dsv};
  }

  void RenderPipeline::SetShaderResource(const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle, const UINT slot) const
  {
    const CD3DX12_CPU_DESCRIPTOR_HANDLE heap_handle
      (
       m_buffer_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
       g_srv_offset + slot,
       m_buffer_descriptor_size_
      );

    GetD3Device().GetDevice()->CopyDescriptorsSimple
      (
       1,
       heap_handle,
       srv_handle,
       D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
      );
  }

  ID3D12RootSignature* RenderPipeline::GetRootSignature() const
  {
    return m_root_signature_.Get();
  }

  void RenderPipeline::SetPSO(const StrongShader& Shader)
  {
    const auto& shader_description = Shader->GetPipelineStateDesc();
    
    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateGraphicsPipelineState
       (
        &shader_description,
        IID_PPV_ARGS(m_pipeline_state_.ReleaseAndGetAddressOf())
       )
      );
  }

  void RenderPipeline::SetMaterial(const CBs::MaterialCB& material_buffer)
  {
    CD3DX12_CPU_DESCRIPTOR_HANDLE heap_handle
      (
       m_buffer_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
       g_srv_offset + slot,
       m_buffer_descriptor_size_
      );

    for (INT i = 0; i < count; ++i)
    {
      const auto& current_handle = heap_handle.Offset(1, m_buffer_descriptor_size_);

      GetD3Device().GetDevice()->CopyDescriptorsSimple
        (
         1,
         current_handle,
         data[i],
         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );
    }
  }

  void RenderPipeline::SetUnorderedAccess(const D3D12_CPU_DESCRIPTOR_HANDLE& uav, const UINT slot) const
  {
    const CD3DX12_CPU_DESCRIPTOR_HANDLE uav_handle
      (
       m_buffer_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
       g_uav_offset + slot,
       m_buffer_descriptor_size_
      );

    GetD3Device().GetDevice()->CopyDescriptorsSimple
      (
       1,
       uav_handle,
       uav,
       D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
      );
  }

  void RenderPipeline::DrawIndexedInstancedDeferred(UINT index_count, UINT instance_count)
  {
    GetD3Device().GetDirectCommandList()->DrawIndexedInstanced(index_count, instance_count, 0, 0, 0);
  }

  void RenderPipeline::TargetDepthOnlyDeferred(const D3D12_CPU_DESCRIPTOR_HANDLE* dsv_handle)
  {
    GetD3Device().GetDirectCommandList()->OMSetRenderTargets(0, nullptr, false, dsv_handle);
  }

  void RenderPipeline::SetViewportDeferred(const D3D12_VIEWPORT& viewport)
  {
    GetD3Device().GetDirectCommandList()->RSSetViewports(1, &viewport);
  }

  void RenderPipeline::CopyBackBuffer(ID3D12Resource* resource) const
  {
    GetD3Device().WaitAndReset(COMMAND_IDX_COPY);

    const auto& copy_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_render_targets_[GetD3Device().GetFrameIndex()].Get(),
       D3D12_RESOURCE_STATE_RENDER_TARGET,
       D3D12_RESOURCE_STATE_COPY_SOURCE
      );

    GetD3Device().GetCopyCommandList()->ResourceBarrier(1, &copy_transition);

    GetD3Device().GetCopyCommandList()->CopyResource(resource, m_render_targets_[GetD3Device().GetFrameIndex()].Get());

    const auto& rtv_transition = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_render_targets_[GetD3Device().GetFrameIndex()].Get(),
       D3D12_RESOURCE_STATE_COPY_SOURCE,
       D3D12_RESOURCE_STATE_RENDER_TARGET
      );

    GetD3Device().GetCopyCommandList()->ResourceBarrier(1, &rtv_transition);

    GetD3Device().ExecuteCopyCommandList();
  }

  ID3D12RootSignature* RenderPipeline::GetRootSignature() const
  {
    return m_root_signature_.Get();
  }

  ID3D12DescriptorHeap* RenderPipeline::GetBufferHeap() const
  {
    return m_buffer_descriptor_heap_.Get();
  }

  ID3D12DescriptorHeap* RenderPipeline::GetSamplerHeap() const
  {
    return m_sampler_descriptor_heap_.Get();
  }

  D3D12_CPU_DESCRIPTOR_HANDLE RenderPipeline::GetCPURTVHandle(UINT index) const
  {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE
      (
       m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
       index,
       m_rtv_descriptor_size_
      );
  }

  D3D12_CPU_DESCRIPTOR_HANDLE RenderPipeline::GetCPUDSVHandle() const
  {
    return m_dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
  }

  D3D12_GPU_DESCRIPTOR_HANDLE RenderPipeline::GetGPURTVHandle(UINT index) const
  {
    return CD3DX12_GPU_DESCRIPTOR_HANDLE
      (
       m_rtv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart(),
       index,
       m_rtv_descriptor_size_
      );
  }

  D3D12_GPU_DESCRIPTOR_HANDLE RenderPipeline::GetGPUDSVHandle() const
  {
    return m_dsv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart();
  }

  D3D12_VIEWPORT RenderPipeline::GetViewport()
  {
    return m_viewport_;
  }

  void RenderPipeline::SetPSO(const StrongShader& Shader)
  {
    const auto& shader_pso = Shader->GetPipelineState();

    GetD3Device().GetDirectCommandList()->SetPipelineState(shader_pso);
  }

  UINT RenderPipeline::GetBufferDescriptorSize() const
  {
    return m_buffer_descriptor_size_;
  }

  UINT RenderPipeline::GetSamplerDescriptorSize() const
  {
    return m_sampler_descriptor_size_;
  }

  void RenderPipeline::UploadConstantBuffers()
  {
    m_wvp_buffer_data_.SetData(&m_wvp_buffer_);
    m_transform_buffer_data_.SetData(&m_transform_buffer_);
    m_material_buffer_data_.SetData(&m_material_buffer_);
    m_param_buffer_data_.SetData(&m_param_buffer_);
  }

  void RenderPipeline::ExecuteDirectCommandList()
  {
    UploadConstantBuffers();
    GetD3Device().ExecuteDirectCommandList();
  }

  void RenderPipeline::SetMaterial(const CBs::MaterialCB& material_buffer)
  {
    m_material_buffer_ = material_buffer;
  }
} // namespace Engine::Manager::Graphics

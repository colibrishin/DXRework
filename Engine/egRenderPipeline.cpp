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
    m_transform_buffer_data_.SetData(&matrix);
  }

  void RenderPipeline::SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix)
  {
    m_wvp_buffer_data_.SetData(&matrix);
  }

  RenderPipeline::TempParamTicket&& RenderPipeline::SetParam(const ParamBase& param)
  {
    TempParamTicket ticket(m_param_buffer_); // RAII

    m_param_buffer_ = static_cast<CBs::ParamCB>(param);
    m_param_buffer_data_.SetData(&m_param_buffer_);
    
    return std::move(ticket);
  }

  void RenderPipeline::DefaultRenderTarget() const
  {
    const auto& rtv_handle = m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
    const auto& dsv_handle = m_dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();

    GetD3Device().GetCommandList()->OMSetRenderTargets(1, &rtv_handle, false, &dsv_handle);
  }

  void RenderPipeline::DefaultViewport() const
  {
    GetD3Device().GetCommandList()->RSSetViewports(1, &m_viewport_);
  }

  void RenderPipeline::SetWireframeState() const
  {
    GetD3Device().GetContext()->RSSetState
      (
       GetToolkitAPI().GetCommonStates()->Wireframe()
      );
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
  
  RenderPipeline::~RenderPipeline() { }

  void RenderPipeline::InitializeStaticBuffers()
  {
    m_wvp_buffer_data_.Create(nullptr);
    m_transform_buffer_data_.Create(nullptr);
    m_material_buffer_data_.Create(nullptr);
    m_param_buffer_data_.Create(nullptr);
  }

  void RenderPipeline::Initialize()
  {
    PrecompileShaders();
    InitializeRootSignature();
    InitializeRenderTargets();
    InitializeDepthStencil();
    InitializeDefaultPSO();
    InitializeHeaps();
    InitializeStaticBuffers();
  }

  void RenderPipeline::PrecompileShaders()
  {
    Shader::Create
      (
       "default", "./default.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS,
       D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
      );

    Shader::Create
      (
       "color", "./color.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
      );

    Shader::Create
      (
       "skybox", "./skybox.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_NONE | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
      );

    Shader::Create
      (
       "specular_normal", "./specular_normal.hlsl",
       SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
      );

    Shader::Create
      (
       "normal", "./normal.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
      );

    Shader::Create
      (
       "refraction", "./refraction.hlsl", SHADER_DOMAIN_POST_PROCESS,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
      );

    Shader::Create
      (
       "specular_tex", "./specular_tex.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
      );

    Shader::Create
      (
       "specular", "./specular.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
      );

    Shader::Create
      (
       "cascade_shadow_stage1", "./cascade_shadow_stage1.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_CLAMP | SHADER_SAMPLER_ALWAYS, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
      );

    const auto billboard = Shader::Create
      (
       "billboard", "./billboard.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_NONE | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT
      );

    Shader::Create
      (
       "intensity_test", "./intensity_test.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_POINT,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_NEVER, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
      );

    Shader::Create
      (
       "atlas", "./atlas.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
      );
  }

  void RenderPipeline::InitializeDefaultPSO()
  {
    
  }

  void RenderPipeline::InitializeRootSignature()
  {
    DirectCommandGuard dcg;

    CD3DX12_DESCRIPTOR_RANGE1 ranges[4];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, CB_TYPE_END, 0);
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, BIND_SLOT_UAV_END, 0);
    ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BIND_SLOT_END, 0);

    CD3DX12_ROOT_PARAMETER1 root_parameters[4];
    root_parameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
    root_parameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
    root_parameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
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

    GetD3Device().GetCommandList()->SetGraphicsRootSignature(m_root_signature_.Get());
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

    const auto rtv_descriptor_size = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize
      (D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

    constexpr D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
    {
      .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
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

      rtv_handle.Offset(1, rtv_descriptor_size);
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

  void RenderPipeline::InitializeHeaps()
  {
    DirectCommandGuard dcg;

    constexpr D3D12_DESCRIPTOR_HEAP_DESC sampler_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
      .NumDescriptors = 2,
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

    constexpr D3D12_DESCRIPTOR_HEAP_DESC cb_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = CB_TYPE_END,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &cb_heap_desc, IID_PPV_ARGS(m_cb_descriptor_heap_.ReleaseAndGetAddressOf())
       )
      );

    constexpr D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = BIND_SLOT_END,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &srv_heap_desc, IID_PPV_ARGS(m_srv_descriptor_heap_.ReleaseAndGetAddressOf())
       )
      );

    constexpr D3D12_DESCRIPTOR_HEAP_DESC uav_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = BIND_SLOT_UAV_END,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0
    };

    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateDescriptorHeap
       (
        &uav_heap_desc, IID_PPV_ARGS(m_uav_descriptor_heap_.ReleaseAndGetAddressOf())
       )
      );

    m_buffer_descriptor_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize
      (D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_rtv_descriptor_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    m_dsv_descriptor_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    m_sampler_descriptor_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    
    ID3D12DescriptorHeap* cb_heaps[]
    {
      m_sampler_descriptor_heap_.Get(),
      m_cb_descriptor_heap_.Get(),
      m_uav_descriptor_heap_.Get(),
      m_srv_descriptor_heap_.Get()
    };

    GetD3Device().GetCommandList()->SetDescriptorHeaps(_countof(cb_heaps), cb_heaps);

    GetD3Device().GetCommandList()->SetGraphicsRootDescriptorTable
      (
       0, m_sampler_descriptor_heap_->GetGPUDescriptorHandleForHeapStart()
      );
  }

  void RenderPipeline::PreUpdate(const float& dt) {}

  void RenderPipeline::PreRender(const float& dt)
  {
    DirectCommandGuard dcg;

    _reset_constant_buffer();
    _reset_structured_buffer();

    constexpr float color[4]   = {0.f, 0.f, 0.f, 1.f};
    const auto&      rtv_handle = m_rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
    const auto&      dsv_handle = m_dsv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
  
    const auto initial_barrier = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_render_targets_[GetD3Device().GetFrameIndex()].Get(),
       D3D12_RESOURCE_STATE_PRESENT,
       D3D12_RESOURCE_STATE_RENDER_TARGET
      );

    GetD3Device().GetCommandList()->OMSetRenderTargets(1, &rtv_handle, false, &dsv_handle);

    GetD3Device().GetCommandList()->ResourceBarrier(1, &initial_barrier);

    GetD3Device().GetCommandList()->ClearRenderTargetView(rtv_handle, color, 0, nullptr);
    GetD3Device().GetCommandList()->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
  }

  void RenderPipeline::Update(const float& dt) {}

  void RenderPipeline::Render(const float& dt)
  {
    GetD3Device().ExecuteDirectCommandList();
  }

  void RenderPipeline::FixedUpdate(const float& dt) {}

  void RenderPipeline::PostRender(const float& dt)
  {
    {
      DirectCommandGuard dcg;

      const auto present_barrier = CD3DX12_RESOURCE_BARRIER::Transition
        (
         m_render_targets_[GetD3Device().GetFrameIndex()].Get(),
         D3D12_RESOURCE_STATE_RENDER_TARGET,
         D3D12_RESOURCE_STATE_PRESENT
        );

      GetD3Device().GetCommandList()->ResourceBarrier(1, &present_barrier);
    }

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

  void RenderPipeline::DrawIndexed(UINT index_count)
  {
    DirectCommandGuard dcg;
    GetD3Device().GetCommandList()->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
  }

  void RenderPipeline::DrawIndexedInstanced(UINT index_count, UINT instance_count)
  {
    DirectCommandGuard dcg;
    GetD3Device().GetCommandList()->DrawIndexedInstanced(index_count, instance_count, 0, 0, 0);
  }

  void RenderPipeline::TargetDepthOnly(const D3D12_CPU_DESCRIPTOR_HANDLE* dsv_handle)
  {
    DirectCommandGuard dcg;
    GetD3Device().GetCommandList()->OMSetRenderTargets(0, nullptr, false, dsv_handle);
  }

  void RenderPipeline::SetViewport(const D3D12_VIEWPORT& viewport)
  {
    GetD3Device().GetCommandList()->RSSetViewports(1, &viewport);
  }

  ID3D12RootSignature* RenderPipeline::GetRootSignature() const
  {
    return m_root_signature_.Get();
  }

  ID3D12DescriptorHeap* RenderPipeline::GetCBHeap() const
  {
    return m_cb_descriptor_heap_.Get();
  }

  void RenderPipeline::SetPSO(const StrongShader& Shader)
  {
    DirectCommandGuard dcg;

    const auto& shader_pso = Shader->GetPipelineState();

    GetD3Device().GetCommandList()->SetPipelineState(shader_pso);
  }

  void RenderPipeline::SetMaterial(const CBs::MaterialCB& material_buffer)
  {
    m_material_buffer_data_.SetData(&material_buffer);
  }
} // namespace Engine::Manager::Graphics
